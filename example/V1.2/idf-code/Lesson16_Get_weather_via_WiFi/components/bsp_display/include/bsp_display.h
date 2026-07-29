#ifndef _BSP_DISPLAY_H_
#define _BSP_DISPLAY_H_
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/clk_tree_defs.h"
#include "esp_lcd_ek79007.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "lvgl.h"
#include "bsp_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define DISPLAY_TAG "DISPLAY"
#define DISPLAY_INFO(fmt, ...) ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_DEBUG(fmt, ...) ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_ERROR(fmt, ...) ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)

#ifdef CONFIG_BSP_DISPLAY_ENABLED

#define V_size CONFIG_V_SIZE
#define H_size CONFIG_H_SIZE
#define BITS_PER_PIXEL CONFIG_BITS_PER_PIXEL

#define LCD_GPIO_BLIGHT CONFIG_LCD_GPIO_BLIGHT
#define BLIGHT_PWM_Hz CONFIG_BLIGHT_PWM_Hz

#define LV_COLOR_RED lv_color_make(0xFF, 0x00, 0x00)
#define LV_COLOR_GREEN lv_color_make(0x00, 0xFF, 0x00)
#define LV_COLOR_BLUE lv_color_make(0x00, 0x00, 0xFF)
#define LV_COLOR_WHITE lv_color_make(0xFF, 0xFF, 0xFF)
#define LV_COLOR_BLACK lv_color_make(0x00, 0x00, 0x00)
#define LV_COLOR_GRAY lv_color_make(0x80, 0x80, 0x80)

#ifdef CONFIG_BSP_TOUCH_ENABLED

#define Touch_GPIO_RST CONFIG_TOUCH_GPIO_RST
#define Touch_GPIO_INT CONFIG_TOUCH_GPIO_INT

esp_err_t touch_read(void);
void set_touch_display(bool state);
void get_coor(uint16_t* x, uint16_t* y, bool* press);
#endif

esp_err_t display_init();
esp_err_t touch_init(void);
esp_err_t get_display_buff(void **buff);
esp_err_t set_lcd_blight(uint32_t brightness);
void fill_screen_with_color(lv_color_t color);
void set_canvas_display(bool state);
#endif
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
