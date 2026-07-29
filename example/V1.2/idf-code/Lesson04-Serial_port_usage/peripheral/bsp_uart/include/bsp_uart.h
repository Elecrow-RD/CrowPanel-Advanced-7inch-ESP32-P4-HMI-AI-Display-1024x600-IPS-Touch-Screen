#ifndef _BSP_UART_H_
#define _BSP_UART_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define UART_TAG "UART"

#define UART_INFO(fmt, ...)   ESP_LOGI(UART_TAG, fmt, ##__VA_ARGS__)
#define UART_DEBUG(fmt, ...)  ESP_LOGD(UART_TAG, fmt, ##__VA_ARGS__)
#define UART_ERROR(fmt, ...)  ESP_LOGE(UART_TAG, fmt, ##__VA_ARGS__)

/*---------------------------------------------------------------
 * Pin assignments
 *--------------------------------------------------------------*/

/* GPIO34/GPIO33 reserved for a second (input-facing) extra UART. */
#define UART_IN_EXTRA_GPIO_TXD 34
#define UART_IN_EXTRA_GPIO_RXD 33

/* UART2 routed to the WiFi module: TX on GPIO47, RX on GPIO48. */
#define UART1_EXTRA_GPIO_TXD 47
#define UART1_EXTRA_GPIO_RXD 48

/*---------------------------------------------------------------
 * Receive state machine (used by higher layers)
 *--------------------------------------------------------------*/
typedef enum {
    UART_SCAN   = 1,   /* Scanning for incoming data. */
    UART_DECODE,       /* Decoding a received frame. */
    UART_ERR,          /* An error occurred. */
} uart_state;

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Send a string over UART2.
 * @param data NUL-terminated string to transmit.
 * @return Number of bytes written.
 */
int SendData(const char *data);

/**
 * @brief Initialise UART2 (baud 115200, 8N1) on GPIO47/GPIO48.
 * @return ESP_OK on success.
 */
esp_err_t uart_init(void);

#endif
