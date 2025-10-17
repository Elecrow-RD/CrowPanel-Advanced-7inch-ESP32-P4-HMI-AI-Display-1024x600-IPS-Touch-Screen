#include "esp_log.h"          // ESP-IDF logging functions
#include "freertos/FreeRTOS.h" // FreeRTOS base header
#include "freertos/task.h"    // FreeRTOS task APIs
#include "bsp_i2c.h"          // Custom I2C BSP driver
#include "bsp_display.h"      // Custom display/touch BSP driver

#define TAG "TOUCH_APP"       // Logging tag for this application

TaskHandle_t touch_task_handle = NULL; // Handle for the touch reading task

// Task function: continuously reads touch data and logs coordinates
void touch_task(void *param)
{
    while (1) {
        if (touch_read() == ESP_OK) {   // Read touch panel
            uint16_t x, y;
            bool pressed;
            get_coor(&x, &y, &pressed); // Get current touch coordinates and state
            
            if (pressed) {
                ESP_LOGI(TAG, "Touch at X=%d, Y=%d", x, y); // Log touch coordinates
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // Delay 50ms between reads
    }
}

// Main application entry point
void app_main(void)
{
    ESP_LOGI(TAG, "Starting touch application"); // Log app start
    
    // Initialize I2C bus
    if (i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed"); // Log error if I2C init fails
        return;
    }

    // Initialize the touchscreen
    if (touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch initialization failed"); // Log error if touch init fails
        return;
    }
    
    // Create a FreeRTOS task for reading touch data
    xTaskCreate(touch_task, "touch_task", 4096, NULL, 5, &touch_task_handle);

    ESP_LOGI(TAG, "Touch application started successfully"); // Log successful start
}
