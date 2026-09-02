/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_uart.h"

/*---------------------------------------------------------------
 * UART2 send / receive helpers
 *--------------------------------------------------------------*/

/**
 * @brief Send a NUL-terminated string over UART2.
 *
 * @param data String to transmit.
 * @return Number of bytes actually written to the UART.
 */
int SendData(const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_2, data, len);
    return txBytes;
}

/**
 * @brief Initialise UART2 used to talk to the WiFi module.
 *
 * Installs the UART driver with a 2 KB receive buffer, maps the UART2
 * signals to GPIO47 (TX) and GPIO48 (RX), and applies the 115200-8N1
 * parameter set.
 *
 * @return ESP_OK on success, an error code otherwise.
 */
esp_err_t uart_init(void)
{
    esp_err_t err = ESP_OK;

    /* 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control. */
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* RX buffer 2 KB; TX buffer 0 means it blocks on the FIFO. */
    err = uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        UART_ERROR("extra uart driver install fail");
        return err;
    }

    /* Map UART2 to the module-facing TX/RX pins. */
    uart_set_pin(UART_NUM_2, UART1_EXTRA_GPIO_TXD, UART1_EXTRA_GPIO_RXD,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    err = uart_param_config(UART_NUM_2, &uart_config);
    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}
