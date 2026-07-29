#include "bsp_uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_log.h"

/* Target WiFi network credentials used by the AT+CWJAP command. */
#define WIFI_SSID "elecrow888"
#define WIFI_PASS "elecrow2014"

/* Upper bound on a single AT response we are willing to buffer. */
#define AT_RESPONSE_MAX 512

static const char *TAG = "WIFI_AT";

/*---------------------------------------------------------------
 * Low-level UART helpers
 *--------------------------------------------------------------*/

/**
 * @brief Read a complete AT response from UART2 within a timeout.
 *
 * Keeps reading short bursts until the overall timeout expires or the
 * buffer is full, then null-terminates the result so it can be treated
 * as a C string.
 *
 * @param buffer  Destination buffer.
 * @param len     Buffer capacity in bytes.
 * @param timeout Maximum wall-clock time to wait (FreeRTOS ticks).
 * @return Total number of bytes stored in the buffer.
 */
static int uart_read_response(char *buffer, size_t len, TickType_t timeout)
{
    int total = 0;
    int read_bytes = 0;
    TickType_t start = xTaskGetTickCount();

    /* Loop until either the timeout elapses or the buffer is full. */
    while ((xTaskGetTickCount() - start) < timeout && total < (int)len - 1) {
        /* Short per-read timeout keeps the loop responsive. */
        read_bytes = uart_read_bytes(UART_NUM_2, (uint8_t *)(buffer + total),
                                     len - total - 1, 20 / portTICK_PERIOD_MS);
        if (read_bytes > 0) {
            total += read_bytes;
        }
    }
    buffer[total] = '\0';
    return total;
}

/*---------------------------------------------------------------
 * AT command helpers
 *--------------------------------------------------------------*/

/**
 * @brief Send an AT command and report whether the module replied OK.
 *
 * Appends the CR+LF terminator expected by AT modules, sends it over
 * UART2, reads back the response and searches it for the "OK" token.
 *
 * @param cmd     AT command string without trailing CR/LF.
 * @param timeout Maximum time to wait for the response.
 * @return true if "OK" was found in the response, false otherwise.
 */
static bool send_at_command(const char *cmd, TickType_t timeout)
{
    char response[AT_RESPONSE_MAX] = {0};

    /* AT commands must end with carriage-return + line-feed. */
    SendData(cmd);
    SendData("\r\n");

    uart_read_response(response, AT_RESPONSE_MAX, timeout);
    ESP_LOGI(TAG, "AT Response: %s", response);

    /* A valid AT response always contains "OK" on success. */
    return strstr(response, "OK") != NULL;
}

/**
 * @brief Issue the join-AP command to connect the module to WiFi.
 *
 * @return true if the module reported a successful connection.
 */
static bool connect_wifi(void)
{
    char cmd[128];

    /* Build the AT+CWJAP command with the SSID and password. */
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);

    /* Joining a network can take a few seconds. */
    if (send_at_command(cmd, pdMS_TO_TICKS(5000))) {
        ESP_LOGI(TAG, "WiFi Connected");
        return true;
    }
    ESP_LOGE(TAG, "Failed to connect WiFi");
    return false;
}

/*---------------------------------------------------------------
 * WiFi control task
 *--------------------------------------------------------------*/

/**
 * @brief Bring up the WiFi module over UART and start a TCP server.
 *
 * Steps: initialise UART2, switch the module to AP+STA mode, reset it,
 * join the configured WiFi network (retrying up to 5 times), query its
 * IP address, enable multiple connections and start a TCP server on
 * port 80.
 *
 * @param arg Unused task argument.
 */
void wifi_task(void *arg)
{
    /* UART must be ready before any AT command can be sent. */
    if (uart_init() != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed");
        vTaskDelete(NULL);
        return;
    }

    /* AP+STA mode lets the module act as both access point and station. */
    send_at_command("AT+CWMODE=3", pdMS_TO_TICKS(1000));
    /* Reset so the new mode takes effect, then give it time to reboot. */
    send_at_command("AT+RST", pdMS_TO_TICKS(2000));
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Retry the join a few times in case the module is still booting. */
    bool connected = false;
    for (int i = 0; i < 5; i++) {
        if (connect_wifi()) {
            connected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    if (!connected) {
        ESP_LOGE(TAG, "Cannot connect to WiFi, stopping task");
        vTaskDelete(NULL);
    }

    /* Query the station IP address assigned by the router. */
    send_at_command("AT+CIFSR", pdMS_TO_TICKS(1000));
    /* Multiple connections are required before starting a server. */
    send_at_command("AT+CIPMUX=1", pdMS_TO_TICKS(1000));
    /* Start a TCP server listening on port 80. */
    send_at_command("AT+CIPSERVER=1,80", pdMS_TO_TICKS(1000));

    while (1) {
        /* Idle loop: incoming TCP data can be polled here. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Creates the WiFi control task with a 4096-byte stack and priority 5.
 */
void app_main(void)
{
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
}
