/**
 * @file bsp_wireless.h
 * @brief Teaching example that demonstrates how to transmit packets with the nRF24 radio.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

#ifndef _BSP_WIRELESS_H
#define _BSP_WIRELESS_H

#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"

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

#define RADIO_GPIO_CLK  8
#define RADIO_GPIO_MISO 7
#define RADIO_GPIO_MOSI 6

#define CONFIG_BSP_SX1262_ENABLED           0
#define CONFIG_BSP_NRF2401_ENABLED          1
#define CONFIG_BSP_UART_TRANSPOND_ENABLED   0
//---------------------------------------------------------------------------
#ifdef CONFIG_BSP_SX1262_ENABLED

#define SX1262_GPIO_BUSY  9
#define SX1262_GPIO_IRQ   27
#define SX1262_GPIO_NRST  28
#define SX1262_GPIO_NSS   10

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Expose SX1262 transmitter initialization to the Arduino lesson code.
     *
     * Parameters: None.
     * @return ESP_OK on success; otherwise an ESP-IDF error code.
     * @note Called during lesson setup before the related hardware is used.
     */
    esp_err_t sx1262_tx_init();
    /**
     * @brief Expose SX1262 transmitter shutdown to the Arduino lesson code.
     *
     * Parameters: None.
     * @return None.
     * @note Called when the related hardware service is being stopped or reconfigured.
     */
    void sx1262_tx_deinit();
    /**
     * @brief Request transmission of the next SX1262 lesson packet.
     *
     * Parameters: None.
     * @return true when the requested operation succeeds or is processed; otherwise false.
     * @note Called when the lesson requests the corresponding data operation.
     */
    bool send_lora_pack_radio();
    
    /**
     * @brief Return the number assigned to the latest SX1262 transmit packet.
     *
     * Parameters: None.
     * @return Current SX1262 transmit counter.
     * @note Called by the lesson workflow when this helper operation is required.
     */
    uint32_t sx1262_get_tx_counter();

    /**
     * @brief Expose SX1262 receiver initialization to the Arduino lesson code.
     *
     * Parameters: None.
     * @return ESP_OK on success; otherwise an ESP-IDF error code.
     * @note Called during lesson setup before the related hardware is used.
     */
    esp_err_t sx1262_rx_init();
    /**
     * @brief Expose SX1262 receiver shutdown to the Arduino lesson code.
     *
     * Parameters: None.
     * @return None.
     * @note Called when the related hardware service is being stopped or reconfigured.
     */
    void sx1262_rx_deinit();
    /**
     * @brief Process a completed SX1262 receive event.
     *
     * @param len Number of bytes available or expected.
     * @return None.
     * @note Called when the lesson needs to obtain or process new data.
     */
    void received_lora_pack_radio(size_t len);
    /**
     * @brief Register the application callback that consumes received SX1262 data.
     *
     * @param callback Function invoked when a complete packet is available.
     * @return None.
     * @note Called by the corresponding event or interrupt path.
     */
    void sx1262_set_rx_callback(void (*callback)(const char* data, size_t len, float rssi, float snr));
    /**
     * @brief Return the length of the latest SX1262 packet.
     *
     * Parameters: None.
     * @return Length of the latest received SX1262 packet in bytes.
     * @note Called when the lesson needs to obtain or process new data.
     */
    size_t sx1262_get_received_len(void);
    /**
     * @brief Report whether the SX1262 receive interrupt has pending data.
     *
     * Parameters: None.
     * @return true while a receive event is waiting to be processed.
     * @note Called when the lesson needs to obtain or process new data.
     */
    bool sx1262_is_data_received(void);
#ifdef __cplusplus
}
#endif

#endif
//---------------------------------------------------------------------------

#ifdef CONFIG_BSP_NRF2401_ENABLED

#define NRF24_GPIO_IRQ  9
#define NRF24_GPIO_CE   27
#define NRF24_GPIO_CS   28

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Expose nRF24 transmitter initialization to the Arduino lesson code.
     *
     * Parameters: None.
     * @return ESP_OK on success; otherwise an ESP-IDF error code.
     * @note Called during lesson setup before the related hardware is used.
     */
    esp_err_t nrf24_tx_init();
    /**
     * @brief Expose nRF24 transmitter shutdown to the Arduino lesson code.
     *
     * Parameters: None.
     * @return None.
     * @note Called when the related hardware service is being stopped or reconfigured.
     */
    void nrf24_tx_deinit();
    /**
     * @brief Request transmission of the next nRF24 lesson packet.
     *
     * Parameters: None.
     * @return true when the requested operation succeeds or is processed; otherwise false.
     * @note Called when the lesson requests the corresponding data operation.
     */
    bool send_nrf24_pack_radio();
    /**
     * @brief Return the current nRF24 transmit counter.
     *
     * Parameters: None.
     * @return Operation result or measured numeric value.
     * @note Called by the lesson workflow when this helper operation is required.
     */
    uint32_t nrf24_get_tx_counter();
    /**
     * @brief Advance the nRF24 transmit counter after a packet is sent.
     *
     * Parameters: None.
     * @return None.
     * @note Called by the lesson workflow when this helper operation is required.
     */
    void nrf24_inc_tx_counter();
    
    /**
     * @brief Expose nRF24 receiver initialization to the Arduino lesson code.
     *
     * Parameters: None.
     * @return ESP_OK on success; otherwise an ESP-IDF error code.
     * @note Called during lesson setup before the related hardware is used.
     */
    esp_err_t nrf24_rx_init();
    /**
     * @brief Expose nRF24 receiver shutdown to the Arduino lesson code.
     *
     * Parameters: None.
     * @return None.
     * @note Called when the related hardware service is being stopped or reconfigured.
     */
    void nrf24_rx_deinit();
    /**
     * @brief Process a completed nRF24 receive event.
     *
     * @param len Number of bytes available or expected.
     * @return None.
     * @note Called when the lesson needs to obtain or process new data.
     */
    void received_nrf24_pack_radio(size_t len);
    /**
     * @brief Register the application callback that consumes received nRF24 data.
     *
     * @param callback Function invoked when a complete packet is available.
     * @return None.
     * @note Called by the corresponding event or interrupt path.
     */
    void nrf24_set_rx_callback(void (*callback)(const char* data, size_t len));
#ifdef __cplusplus
}
#endif
#endif
//---------------------------------------------------------------------------

#ifdef CONFIG_BSP_UART_TRANSPOND_ENABLED

#define UART_GPIO_TXD 27
#define UART_GPIO_RXD 28
#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Initialize the optional UART pass-through channel.
     *
     * Parameters: None.
     * @return ESP_OK on success; otherwise an ESP-IDF error code.
     * @note Called during lesson setup before the related hardware is used.
     */
    esp_err_t uart_transpond_init();
    /**
     * @brief Release the optional UART pass-through channel.
     *
     * Parameters: None.
     * @return None.
     * @note Called when the related hardware service is being stopped or reconfigured.
     */
    void uart_transpond_deinit();
#ifdef __cplusplus
}
#endif
#endif

//---------------------------------------------------------------------------
#endif