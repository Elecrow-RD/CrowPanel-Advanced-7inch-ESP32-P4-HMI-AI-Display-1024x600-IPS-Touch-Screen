/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"
#include <string.h>
#include <esp_log.h>                 // ESP-IDF logging library
#include <esp_err.h>                 // ESP-IDF error codes
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("ERROR"); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_WARN(fmt, ...)       do { \
                                        Serial.print("WARN: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_INFO(fmt, ...)       do { \
                                        Serial.print("INFO: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_DEBUG(fmt, ...)      do { \
                                        Serial.print("DEBUG: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  

#define WIFI_SSID "elecrow888"  // WiFi network name
#define WIFI_PASS "elecrow2014"  // WiFi network password

#define AT_RESPONSE_MAX 512  // Maximum length for AT command responses

/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
static const char *TAG = "WIFI_AT";  // Tag for logging messages

/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */
esp_err_t uart_init()
{
    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

    // Print to the debug console
    Serial.println("Serial 0 (Debug) initialized.");

    // Initialize Serial1 (UART1) with custom pins
    // Parameters: baud rate, config, RX pin, TX pin
    Serial1.begin(115200, SERIAL_8N1, UART1_EXTRA_GPIO_RXD, UART1_EXTRA_GPIO_TXD);

    // Print to the second serial port
    Serial.println("Serial 1 (External Device) initialized.");

    return ESP_OK;                // Return success if everything is OK
}

static int SendData(const char *data)        // Function to send a string of data through UART2
{
    const int len = strlen(data);     // Get the length of the input string
    const int txBytes = Serial1.write(data, len);  // Write string to UART2
    return txBytes;                   // Return number of bytes actually sent
}

/* Read UART return data */
static int uart_read_response(char *buffer, size_t len, TickType_t timeout)
{
    int total = 0;  // Total number of bytes read
    int read_bytes = 0;  // Bytes read in current iteration
    TickType_t start = xTaskGetTickCount();  // Get current system tick count
    // Continue reading until timeout or buffer is full
    while ((xTaskGetTickCount() - start) < timeout && total < len - 1)
    {
        // Read bytes from UART2
        if (Serial1.available()) {
            read_bytes = Serial1.read((uint8_t *)(buffer + total), len - total - 1);
            if (read_bytes > 0)
            {
                total += read_bytes;  // Accumulate total bytes read
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    buffer[total] = '\0';  // Null-terminate the response string
    return total;  // Return total bytes read
}

/* Send AT command and wait for OK response */
static bool send_at_command(const char *cmd, TickType_t timeout)
{
    char response[AT_RESPONSE_MAX] = {0};  // Buffer to store response
    SendData(cmd);  // Send the AT command
    SendData("\r\n");  // Send command terminator

    uart_read_response(response, AT_RESPONSE_MAX, timeout);  // Read response
    PRINTF_INFO("AT Response: %s", response);  // Log the response

    // Check if response contains "OK"
    if (strstr(response, "OK") != NULL)
        return true;  // Command succeeded
    else
        return false;  // Command failed
}

/* WiFi connection function */
static bool connect_wifi()
{
    char cmd[128];  // Buffer to build AT command

    // Construct AT command to join WiFi network
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    PRINTF_INFO("Connecting to WiFi: %s", WIFI_SSID);  // Log connection attempt

    // Send command with 5 second timeout and return result
    if (send_at_command(cmd, pdMS_TO_TICKS(5000)))
    {
        PRINTF_INFO("WiFi Connected");  // Log successful connection
        return true;
    }
    else
    {
        PRINTF_ERROR("Failed to connect WiFi");  // Log connection failure
        return false;
    }
}

void wifi_task(void *arg)
{
    // Initialize UART communication
    if (uart_init() != ESP_OK)
    {
        PRINTF_ERROR("UART init failed");  // Log UART initialization failure
        vTaskDelete(NULL);  // Delete current task if initialization fails
        return;
    }

    // Configure module to AP+STA mode (Access Point + Station)
    send_at_command("AT+CWMODE=3", pdMS_TO_TICKS(1000));
    // Reset the module to apply settings
    send_at_command("AT+RST", pdMS_TO_TICKS(2000));
    vTaskDelay(pdMS_TO_TICKS(3000));  // Delay to allow module to restart

    // Attempt to connect to WiFi, maximum 5 tries
    bool connected = false;
    for (int i = 0; i < 5; i++)
    {
        if (connect_wifi())
        {
            connected = true;  // Mark as connected if successful
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));  // Delay between connection attempts
    }

    if (!connected)
    {
        PRINTF_ERROR(TAG, "Cannot connect to WiFi, stopping task");  // Log failure after all attempts
        vTaskDelete(NULL);  // Delete task if connection failed
    }

    // Get IP address of the module
    send_at_command("AT+CIFSR", pdMS_TO_TICKS(1000));
    // Enable multiple connections mode
    send_at_command("AT+CIPMUX=1", pdMS_TO_TICKS(1000));
    // Start TCP server on port 80
    send_at_command("AT+CIPSERVER=1,80", pdMS_TO_TICKS(1000));

    PRINTF_INFO("Complete the Wi-Fi connection task.");

    while (1)
    {
        // TODO: Can read UART data here to process TCP requests
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay to reduce CPU usage
    }
}

void setup() {
  // put your setup code here, to run once:

  // Create WiFi task with 4096 bytes stack, priority 5
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
    
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
