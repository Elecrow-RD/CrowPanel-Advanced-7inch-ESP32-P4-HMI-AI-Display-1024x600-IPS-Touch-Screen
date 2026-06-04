#include "bsp_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_log.h"

#define WIFI_SSID "elecrow888"  // WiFi network name
#define WIFI_PASS "elecrow2014"  // WiFi network password

#define AT_RESPONSE_MAX 512  // Maximum length for AT command responses
static const char *TAG = "WIFI_AT";  // Tag for logging messages

/* Read UART return data */
static int uart_read_response(char *buffer, size_t len, TickType_t timeout)
{
    int total = 0;  // Total number of bytes read
    int read_bytes = 0;  // Bytes read in current iteration
    TickType_t start = xTaskGetTickCount();  // Get current system tick count
    // Continue reading until timeout or buffer is full
    while ((xTaskGetTickCount() - start) < timeout && total < len - 1)
    {
        // Read bytes from UART2 with 20ms timeout per read
        read_bytes = uart_read_bytes(UART_NUM_2, (uint8_t *)(buffer + total), len - total - 1, 20 / portTICK_PERIOD_MS);
        if (read_bytes > 0)
        {
            total += read_bytes;  // Accumulate total bytes read
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
    ESP_LOGI(TAG, "AT Response: %s", response);  // Log the response

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
    ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);  // Log connection attempt

    // Send command with 5 second timeout and return result
    if (send_at_command(cmd, pdMS_TO_TICKS(5000)))
    {
        ESP_LOGI(TAG, "WiFi Connected");  // Log successful connection
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect WiFi");  // Log connection failure
        return false;
    }
}

void wifi_task(void *arg)
{
    // Initialize UART communication
    if (uart_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "UART init failed");  // Log UART initialization failure
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
        ESP_LOGE(TAG, "Cannot connect to WiFi, stopping task");  // Log failure after all attempts
        vTaskDelete(NULL);  // Delete task if connection failed
    }

    // Get IP address of the module
    send_at_command("AT+CIFSR", pdMS_TO_TICKS(1000));
    // Enable multiple connections mode
    send_at_command("AT+CIPMUX=1", pdMS_TO_TICKS(1000));
    // Start TCP server on port 80
    send_at_command("AT+CIPSERVER=1,80", pdMS_TO_TICKS(1000));

    while (1)
    {
        // TODO: Can read UART data here to process TCP requests
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay to reduce CPU usage
    }
}

void app_main(void)
{
    // Create WiFi task with 4096 bytes stack, priority 5
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
}