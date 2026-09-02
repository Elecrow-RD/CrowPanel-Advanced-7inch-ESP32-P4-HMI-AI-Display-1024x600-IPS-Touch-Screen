/**
 * @file bsp_dht20.h
 * @brief Teaching example that demonstrates how to read a DHT20 sensor and display its measurements with LVGL.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

#ifndef _BSP_DHT20_H_
#define _BSP_DHT20_H_

#include <string.h>            // Standard memory and string helpers.
#include "freertos/FreeRTOS.h" // FreeRTOS types and timing helpers.
#include "freertos/task.h"     // FreeRTOS task API.
#include "esp_log.h"           // ESP-IDF logging API.
#include "esp_err.h"           // ESP-IDF error-code definitions.
#include "esp_timer.h"         // ESP-IDF high-resolution timer API.
#include "bsp_i2c.h"
#ifdef __cplusplus 
extern "C" {
#endif  //__cplusplus

#define DHT20_TAG "DHT20"
#define DHT20_INFO(fmt, ...) ESP_LOGI(DHT20_TAG, fmt, ##__VA_ARGS__)
#define DHT20_DEBUG(fmt, ...) ESP_LOGD(DHT20_TAG, fmt, ##__VA_ARGS__)
#define DHT20_ERROR(fmt, ...) ESP_LOGE(DHT20_TAG, fmt, ##__VA_ARGS__)

#define DHT20_I2C_ADDRESS 0x38 // The 7-bit I2C address of DHT20

#define DHT20_MEASURE_TIMEOUT 1000 // Measurement timeout time of DHT20

typedef struct dht20_data
{
    float temperature;  // The measured temperature data
    float humidity;     // the measured humidity data
    uint32_t raw_humid; // Intermediate quantity for humidity data conversion
    uint32_t raw_temp;  // Intermediate quantity for temperature data conversion
} dht20_data_t;

/**
 * @brief Prepare the DHT20 sensor and verify that its calibration state can be restored.
 *
 * Parameters: None.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called by the lesson workflow when this helper operation is required.
 */
esp_err_t dht20_begin(void);                   // Initialization of DHT20 sensor
/**
 * @brief Check whether the DHT20 calibration bits indicate a ready sensor.
 *
 * Parameters: None.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called by the lesson workflow when this helper operation is required.
 */
esp_err_t dht20_is_calibrated(void);           // The function for determining whether the DHT20 sensor is ready or not
/**
 * @brief Trigger a DHT20 measurement, validate it, and convert the raw values.
 *
 * @param data Output structure that receives raw and converted measurements.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson needs to obtain or process new data.
 */
esp_err_t dht20_read_data(dht20_data_t *data); // DHT20 Sensor Temperature and Humidity Data Reading Function

#ifdef __cplusplus 
}
#endif  //__cplusplus
#endif
