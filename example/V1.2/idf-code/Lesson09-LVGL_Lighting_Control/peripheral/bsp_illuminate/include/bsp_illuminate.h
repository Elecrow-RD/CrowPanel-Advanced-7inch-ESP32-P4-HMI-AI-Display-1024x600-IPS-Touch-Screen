#ifndef _BSP_ILLUMINATE_H_
#define _BSP_ILLUMINATE_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_ek79007.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "lvgl.h"
#include "bsp_display.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define ILLUMINATE_TAG "ILLUMINATE"

#define ILLUMINATE_INFO(fmt, ...)   ESP_LOGI(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_DEBUG(fmt, ...)  ESP_LOGD(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_ERROR(fmt, ...) ESP_LOGE(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)

/* Panel resolution 1024 x 600, 16 bpp (RGB565). */
#define V_size 600
#define H_size 1024
#define BITS_PER_PIXEL 16

/* Backlight PWM on GPIO31 at 30 kHz. */
#define LCD_GPIO_BLIGHT 31
#define BLIGHT_PWM_Hz 30000

/* Convenience colour macros for LVGL. */
#define LV_COLOR_RED    lv_color_make(0xFF, 0x00, 0x00)
#define LV_COLOR_GREEN  lv_color_make(0x00, 0xFF, 0x00)
#define LV_COLOR_BLUE   lv_color_make(0x00, 0x00, 0xFF)
#define LV_COLOR_WHITE  lv_color_make(0xFF, 0xFF, 0xFF)
#define LV_COLOR_BLACK  lv_color_make(0x00, 0x00, 0x00)
#define LV_COLOR_GRAY   lv_color_make(0x80, 0x80, 0x80)
#define LV_COLOR_YELLOW lv_color_make(0xFF, 0xFF, 0x00)

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Bring up the backlight, MIPI DSI panel, LVGL display and touch.
 * @return ESP_OK on success.
 */
esp_err_t display_init(void);

/**
 * @brief Set the backlight brightness (0-100).
 * @param brightness 0 = off, 100 = max.
 * @return ESP_OK on success.
 */
esp_err_t set_lcd_blight(uint32_t brightness);

#endif
