#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp_i2c.h"
#include "bsp_display.h"

#define TAG "TOUCH_APP"

/* Handle of the task that polls the touch panel. */
static TaskHandle_t touch_task_handle = NULL;

/*---------------------------------------------------------------
 * Touch reading task
 *--------------------------------------------------------------*/

/**
 * @brief Periodically read the touch panel and log coordinates.
 *
 * Polls the GT911 every 50 ms; whenever a press is detected the
 * current X/Y coordinates are printed to the serial console so the
 * learner can verify the touch link end to end.
 *
 * @param param Unused task argument.
 */
static void touch_task(void *param)
{
    while (1) {
        if (touch_read() == ESP_OK) {
            uint16_t x, y;
            bool pressed;
            get_coor(&x, &y, &pressed);

            if (pressed) {
                ESP_LOGI(TAG, "Touch at X=%d, Y=%d", x, y);
            }
        }
        /* 50 ms polling interval balances responsiveness and CPU load. */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Brings up the I2C bus first (the touch controller hangs on it),
 * then initialises the GT911 touch panel and starts the reading task.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting touch application");

    /* The I2C bus must exist before the touch driver can talk to GT911. */
    if (i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }

    if (touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch initialization failed");
        return;
    }

    xTaskCreate(touch_task, "touch_task", 4096, NULL, 5, &touch_task_handle);
    ESP_LOGI(TAG, "Touch application started successfully");
}
