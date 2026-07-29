#ifndef _BSP_DISPLAY_H_
#define _BSP_DISPLAY_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_touch_gt911.h"
#include "bsp_i2c.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define DISPLAY_TAG "DISPLAY"

#define DISPLAY_INFO(fmt, ...)   ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_DEBUG(fmt, ...)  ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_ERROR(fmt, ...)  ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)

/*---------------------------------------------------------------
 * Touch panel geometry and pins
 *--------------------------------------------------------------*/

/* Touch resolution must match the physical panel (1024 x 600). */
#define H_size 1024
#define V_size 600

/* GT911 reset and interrupt pins. */
#define Touch_GPIO_RST 40
#define Touch_GPIO_INT 42

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Read the latest touch coordinates and press flag.
 */
void get_coor(uint16_t *x, uint16_t *y, bool *press);

/**
 * @brief Initialise the GT911 touch panel.
 * @return ESP_OK on success.
 */
esp_err_t touch_init(void);

/**
 * @brief Read one sample and update the cached coordinates.
 * @return ESP_OK on success.
 */
esp_err_t touch_read(void);

#endif
