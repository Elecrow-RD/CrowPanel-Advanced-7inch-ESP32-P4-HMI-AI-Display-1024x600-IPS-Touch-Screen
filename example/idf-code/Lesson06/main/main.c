#include "esp_log.h"          // ESP-IDF logging library for debug/info/error messages
#include "freertos/FreeRTOS.h" // FreeRTOS base definitions
#include "freertos/task.h"    // FreeRTOS task management API
#include "bsp_i2c.h"          // Custom I2C driver for touch controller
#include "bsp_display.h"      // Custom display/touch BSP driver
#include "bsp_usb.h"          // Custom USB HID BSP driver

#define TAG "TOUCH_MOUSE"     // Log tag used for this module
TaskHandle_t touch_task_handle = NULL; // Task handle for the touch-to-mouse task

void touch_mouse_task(void *param) // Task function to handle touch input and emulate mouse movement
{
    ESP_LOGI(TAG, "Touch mouse task started"); // Log task start
    
    uint16_t prev_x = 0xffff;   // Previous X coordinate (initialized to invalid value)
    uint16_t prev_y = 0xffff;   // Previous Y coordinate (initialized to invalid value)
    bool prev_pressed = false;  // Previous touch state (pressed or not)
    
    while (1) { // Infinite loop for continuous task execution
        if (touch_read() == ESP_OK) { // Read touch input
            uint16_t x, y;       // Current X, Y coordinates
            bool pressed;        // Current touch state
            get_coor(&x, &y, &pressed); // Retrieve touch coordinates and state
            
            // Send mouse movement only when USB is ready and screen is being touched
            if (pressed && is_usb_ready()) {
                if (prev_pressed && prev_x != 0xffff && prev_y != 0xffff) {
                    // Calculate movement delta
                    int16_t delta_x = (int16_t)x - (int16_t)prev_x;
                    int16_t delta_y = (int16_t)y - (int16_t)prev_y;
                    
                    // Send mouse movement HID report
                    send_hid_mouse_delta(delta_x, delta_y);
                    ESP_LOGI(TAG, "Mouse move: ΔX=%d, ΔY=%d", delta_x, delta_y); // Log movement
                }
                
                prev_x = x;  // Update previous X
                prev_y = y;  // Update previous Y
            } else if (!pressed) {
                // Reset previous coordinates when touch is released
                prev_x = 0xffff;
                prev_y = 0xffff;
            }
            
            prev_pressed = pressed; // Save current press state
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay 10ms → 100Hz sampling rate
    }
}

void app_main(void) // Main application entry point
{
    ESP_LOGI(TAG, "Starting Touch Mouse application"); // Log application start
    
    // Initialize I2C bus
    if (i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed"); // Log error if I2C init fails
        return;
    }

    // Initialize touchscreen
    if (touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch initialization failed"); // Log error if touch init fails
        return;
    }

    // Initialize USB HID subsystem
    if (usb_init() != ESP_OK) {
        ESP_LOGE(TAG, "USB initialization failed"); // Log error if USB init fails
        return;
    }
    
    // Create FreeRTOS task for touch-to-mouse handling
    xTaskCreate(touch_mouse_task, "touch_mouse_task", 4096, NULL, 5, &touch_task_handle);
    if (touch_task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create touch mouse task"); // Log error if task creation fails
        return;
    }

    ESP_LOGI(TAG, "Touch Mouse application started successfully"); // Log successful startup
}
