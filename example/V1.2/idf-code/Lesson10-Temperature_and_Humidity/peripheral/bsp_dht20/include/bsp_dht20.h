#ifndef _BSP_DHT20_H_
#define _BSP_DHT20_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "bsp_i2c.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define DHT20_TAG "DHT20"

#define DHT20_INFO(fmt, ...)   ESP_LOGI(DHT20_TAG, fmt, ##__VA_ARGS__)
#define DHT20_DEBUG(fmt, ...)  ESP_LOGD(DHT20_TAG, fmt, ##__VA_ARGS__)
#define DHT20_ERROR(fmt, ...)  ESP_LOGE(DHT20_TAG, fmt, ##__VA_ARGS__)

/* DHT20 7-bit I2C address. */
#define DHT20_I2C_ADDRESS 0x38

/* Maximum time to wait for a measurement, in milliseconds. */
#define DHT20_MEASURE_TIMEOUT 1000

/*---------------------------------------------------------------
 * Data types
 *--------------------------------------------------------------*/

/* Converted and raw sensor readings. */
typedef struct dht20_data {
    float temperature;   /* Degrees Celsius. */
    float humidity;       /* Percent RH. */
    uint32_t raw_humid;  /* 20-bit raw humidity. */
    uint32_t raw_temp;   /* 20-bit raw temperature. */
} dht20_data_t;

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the DHT20 sensor (register I2C device + calibrate).
 * @return ESP_OK on success.
 */
esp_err_t dht20_begin(void);

/**
 * @brief Check whether the sensor reports calibrated status.
 * @return ESP_OK if calibrated, ESP_FAIL otherwise.
 */
esp_err_t dht20_is_calibrated(void);

/**
 * @brief Trigger a measurement and read temperature/humidity.
 * @param data Output structure receiving the converted values.
 * @return ESP_OK on success.
 */
esp_err_t dht20_read_data(dht20_data_t *data);

#endif
