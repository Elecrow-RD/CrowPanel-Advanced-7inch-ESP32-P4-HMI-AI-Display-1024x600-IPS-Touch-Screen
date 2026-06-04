/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_uart.h"                 // Include the corresponding UART header file
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

int SendData(const char *data)        // Function to send a string of data through UART2
{
    const int len = strlen(data);     // Get the length of the input string
    const int txBytes = uart_write_bytes(UART_NUM_2, data, len);  // Write string to UART2
    return txBytes;                   // Return number of bytes actually sent
}

esp_err_t uart_init()                 // Function to initialize UART2
{
    esp_err_t err = ESP_OK;           // Variable to store error status, default to ESP_OK
    const uart_config_t uart_config = {   // UART configuration structure
        .baud_rate = 115200,                   // Set baud rate to 115200
        .data_bits = UART_DATA_8_BITS,         // 8 data bits per frame
        .parity = UART_PARITY_DISABLE,         // Disable parity check
        .stop_bits = UART_STOP_BITS_1,         // 1 stop bit
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // Disable hardware flow control
        .source_clk = UART_SCLK_DEFAULT,       // Use default UART clock source
    };

    err = uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);   // Install UART2 driver with RX buffer size 2048 bytes
    if (err != ESP_OK)             // Check if driver installation failed
    {
        UART_ERROR("extra uart driver install fail");   // Log error if installation failed
        return err;                                    // Return error code
    }
    uart_set_pin(UART_NUM_2, UART1_EXTRA_GPIO_TXD, UART1_EXTRA_GPIO_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  
    // Configure UART2 TX and RX pins, keep RTS/CTS unchanged
    err = uart_param_config(UART_NUM_2, &uart_config);  // Apply UART parameter configuration
    if (err != ESP_OK)                                  // Check if configuration failed
        return err;                                     // Return error code

    return ESP_OK;                // Return success if everything is OK
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
