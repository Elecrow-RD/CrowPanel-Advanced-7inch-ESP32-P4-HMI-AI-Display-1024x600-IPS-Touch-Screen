#ifndef _BSP_EXTRA_H_
#define _BSP_EXTRA_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define EXTRA_TAG "EXTRA"

#define EXTRA_INFO(fmt, ...)   ESP_LOGI(EXTRA_TAG, fmt, ##__VA_ARGS__)
#define EXTRA_DEBUG(fmt, ...)  ESP_LOGD(EXTRA_TAG, fmt, ##__VA_ARGS__)
#define EXTRA_ERROR(fmt, ...) ESP_LOGE(EXTRA_TAG, fmt, ##__VA_ARGS__)

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the LED GPIO (GPIO48) as an output.
 * @return ESP_OK on success.
 */
esp_err_t gpio_extra_init(void);

/**
 * @brief Set the LED pin level.
 * @param level true = LED on, false = LED off.
 * @return ESP_OK.
 */
esp_err_t gpio_extra_set_level(bool level);

#endif
