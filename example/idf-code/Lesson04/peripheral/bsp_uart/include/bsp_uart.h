#ifndef _BSP_UART_H_                // Prevent multiple inclusion of this header file
#define _BSP_UART_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>                  // Standard C library for string manipulation
#include <stdint.h>                  // Standard integer type definitions (e.g., uint8_t, int32_t)
#include "freertos/FreeRTOS.h"       // FreeRTOS core definitions
#include "freertos/task.h"           // FreeRTOS task management APIs
#include "esp_log.h"                 // ESP-IDF logging library
#include "esp_err.h"                 // ESP-IDF error codes
#include "driver/uart.h"             // ESP-IDF UART driver APIs
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define UART_TAG "UART"              // Logging tag for UART module
#define UART_INFO(fmt, ...) ESP_LOGI(UART_TAG, fmt, ##__VA_ARGS__)   // Macro for UART info log
#define UART_DEBUG(fmt, ...) ESP_LOGD(UART_TAG, fmt, ##__VA_ARGS__)  // Macro for UART debug log
#define UART_ERROR(fmt, ...) ESP_LOGE(UART_TAG, fmt, ##__VA_ARGS__)  // Macro for UART error log

#define UART_IN_EXTRA_GPIO_TXD 34    // Define GPIO number 34 as UART TXD pin for input extra UART
#define UART_IN_EXTRA_GPIO_RXD 33    // Define GPIO number 33 as UART RXD pin for input extra UART

#define UART1_EXTRA_GPIO_TXD 47      // Define GPIO number 47 as UART1 TXD pin
#define UART1_EXTRA_GPIO_RXD 48      // Define GPIO number 48 as UART1 RXD pin

typedef enum
{
    UART_SCAN = 1,                   // UART state: scanning or waiting for input
    UART_DECODE,                     // UART state: decoding received data
    UART_ERR,                        // UART state: error occurred
} uart_state;

int SendData(const char *data);      // Function to send data over UART
esp_err_t uart_init();               // Function to initialize UART

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif                               // End of header guard
