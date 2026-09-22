#ifndef _BSP_DISPLAY_H_   // Header guard start: prevent multiple inclusion
#define _BSP_DISPLAY_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"                 // ESP-IDF logging functions
#include "esp_err.h"                 // ESP-IDF error codes (esp_err_t)
#include "freertos/FreeRTOS.h"       // FreeRTOS base header
#include "freertos/task.h"           // FreeRTOS task APIs
#include "esp_lcd_touch_gt911.h"     // GT911 touch driver APIs
#include "bsp_i2c.h"                 // Custom I2C BSP driver
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define DISPLAY_TAG "DISPLAY"  // Logging tag for display-related logs

// Macros for logging at different levels with the DISPLAY tag
#define DISPLAY_INFO(fmt, ...) ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)  
#define DISPLAY_DEBUG(fmt, ...) ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)  
#define DISPLAY_ERROR(fmt, ...) ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)  

// Touch panel resolution
#define V_size 600      // Vertical resolution (Y-axis)
#define H_size 1024     // Horizontal resolution (X-axis)

// GPIO pins for GT911 touch panel
#define Touch_GPIO_RST 40   // Reset pin
#define Touch_GPIO_INT 42   // Interrupt pin

extern esp_lcd_touch_handle_t tp;

// Public API: Get the latest touch coordinates and press state
void get_coor(uint16_t* x, uint16_t* y, bool* press);

// Internal API: Update the stored coordinates and press state (only used inside .c file)
void set_coor(uint16_t x, uint16_t y, bool press);

// Initialize the GT911 touch panel
esp_err_t touch_init(void);

// Read the touch panel data and update coordinates
esp_err_t touch_read(void);
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif   // End of header guard
