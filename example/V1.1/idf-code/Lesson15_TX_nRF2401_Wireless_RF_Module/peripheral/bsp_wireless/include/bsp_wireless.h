#ifndef _BSP_WIRELESS_H
#define _BSP_WIRELESS_H

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define SX1262_TAG "SX1262"
#define SX1262_INFO(fmt, ...) ESP_LOGI(SX1262_TAG, fmt, ##__VA_ARGS__)
#define SX1262_DEBUG(fmt, ...) ESP_LOGD(SX1262_TAG, fmt, ##__VA_ARGS__)
#define SX1262_ERROR(fmt, ...) ESP_LOGE(SX1262_TAG, fmt, ##__VA_ARGS__)

#define NRF2401_TAG "NRF2401"
#define NRF2401_INFO(fmt, ...) ESP_LOGI(NRF2401_TAG, fmt, ##__VA_ARGS__)
#define NRF2401_DEBUG(fmt, ...) ESP_LOGD(NRF2401_TAG, fmt, ##__VA_ARGS__)
#define NRF2401_ERROR(fmt, ...) ESP_LOGE(NRF2401_TAG, fmt, ##__VA_ARGS__)

#define WIRELESS_UART_TAG "WIRELESS_UART"
#define WIRELESS_UART_INFO(fmt, ...) ESP_LOGI(WIRELESS_UART_TAG, fmt, ##__VA_ARGS__)
#define WIRELESS_UART_DEBUG(fmt, ...) ESP_LOGD(WIRELESS_UART_TAG, fmt, ##__VA_ARGS__)
#define WIRELESS_UART_ERROR(fmt, ...) ESP_LOGE(WIRELESS_UART_TAG, fmt, ##__VA_ARGS__)

#define RADIO_GPIO_CLK 8
#define RADIO_GPIO_MISO 7
#define RADIO_GPIO_MOSI 6

//---------------------------------------------------------------------------
#ifdef CONFIG_BSP_SX1262_ENABLED

#define SX1262_GPIO_BUSY 9
#define SX1262_GPIO_IRQ 53
#define SX1262_GPIO_NRST 54
#define SX1262_GPIO_NSS 10

#ifdef __cplusplus
extern "C"
{
#endif
    esp_err_t sx1262_tx_init();
    void sx1262_tx_deinit();
    bool send_lora_pack_radio();
    
    uint32_t sx1262_get_tx_counter();

    esp_err_t sx1262_rx_init();
    void sx1262_rx_deinit();
    void received_lora_pack_radio(size_t len);
    void sx1262_set_rx_callback(void (*callback)(const char* data, size_t len, float rssi, float snr));
    size_t sx1262_get_received_len(void);
    bool sx1262_is_data_received(void);
#ifdef __cplusplus
}
#endif

#endif
//---------------------------------------------------------------------------

#ifdef CONFIG_BSP_NRF2401_ENABLED

#define NRF24_GPIO_IRQ 9
#define NRF24_GPIO_CE 53
#define NRF24_GPIO_CS 54

#ifdef __cplusplus
extern "C"
{
#endif
    esp_err_t nrf24_tx_init();
    void nrf24_tx_deinit();
    bool send_nrf24_pack_radio();
    uint32_t nrf24_get_tx_counter();
    void nrf24_inc_tx_counter();
    
    esp_err_t nrf24_rx_init();
    void nrf24_rx_deinit();
    void received_nrf24_pack_radio(size_t len);
    void nrf24_set_rx_callback(void (*callback)(const char* data, size_t len));
#ifdef __cplusplus
}
#endif
#endif
//---------------------------------------------------------------------------

#ifdef CONFIG_BSP_UART_TRANSPOND_ENABLED

#define UART_GPIO_TXD 53
#define UART_GPIO_RXD 54
#ifdef __cplusplus
extern "C"
{
#endif
    esp_err_t uart_transpond_init();
    void uart_transpond_deinit();
#ifdef __cplusplus
}
#endif
#endif

//---------------------------------------------------------------------------
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif