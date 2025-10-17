#ifndef _BSP_ILLUMINATE_H_
#define _BSP_ILLUMINATE_H_
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"           //References for LOG Printing Function-related API Functions
#include "esp_err.h"           //References for Error Type Function-related API Functions
#include "esp_ldo_regulator.h" //References for LDO Function-related API Functions
#include "esp_lcd_ek79007.h"   //References for lcd ek79007 Function-related API Functions
#include "esp_lcd_mipi_dsi.h"  //References for lcd mipi dsi Function-related API Functions
#include "esp_lcd_panel_ops.h" //References for lcd panel ops Function-related API Functions
#include "esp_lcd_panel_io.h"  //References for lcd panel io Function-related API Functions
#include "esp_lvgl_port.h"     //References for LVGL port Function-related API Functions
#include "driver/gpio.h"       //References for GPIO Function-related API Functions
#include "driver/ledc.h"       //References for LEDC PWM Function-related API Functions
#include "lvgl.h"              //References for LVGL Function-related API Functions
#include "bsp_display.h"   
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define ILLUMINATE_TAG "ILLUMINATE"
#define ILLUMINATE_INFO(fmt, ...) ESP_LOGI(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_DEBUG(fmt, ...) ESP_LOGD(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_ERROR(fmt, ...) ESP_LOGE(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)

#define V_size 600              // Vertical resolution
#define H_size 1024             // Horizontal resolution
#define BITS_PER_PIXEL 16       // Number of image display bits of the display screen

#define LCD_GPIO_BLIGHT 31      // LCD Blight GPIO
#define BLIGHT_PWM_Hz 30000     // LCD Blight PWM GPIO

#define LV_COLOR_RED lv_color_make(0xFF, 0x00, 0x00)    // LVGL Red
#define LV_COLOR_GREEN lv_color_make(0x00, 0xFF, 0x00)  // LVGL Green
#define LV_COLOR_BLUE lv_color_make(0x00, 0x00, 0xFF)   // LVGL Blue
#define LV_COLOR_WHITE lv_color_make(0xFF, 0xFF, 0xFF)  // LVGL White
#define LV_COLOR_BLACK lv_color_make(0x00, 0x00, 0x00)  // LVGL Black
#define LV_COLOR_GRAY lv_color_make(0x80, 0x80, 0x80)   // LVGL gray
#define LV_COLOR_YELLOW lv_color_make(0xFF, 0xFF, 0x00) // LVGL yellow

esp_err_t display_init();                      // Display Screen Initialization Function
esp_err_t set_lcd_blight(uint32_t brightness); // Set the screen backlight

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
