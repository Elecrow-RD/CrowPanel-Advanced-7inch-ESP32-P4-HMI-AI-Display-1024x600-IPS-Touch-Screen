#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_usb.h"

#define TAG "TOUCH_MOUSE"

/* Handle of the task that turns touch input into mouse movement. */
static TaskHandle_t touch_task_handle = NULL;

/*---------------------------------------------------------------
 * Touch-to-mouse task
 *--------------------------------------------------------------*/

/**
 * @brief Convert touch motion into relative mouse movement over USB HID.
 *
 * Samples the touch panel at 100 Hz. While a finger is down and the
 * USB host is ready, the movement delta since the previous sample is
 * sent as an HID mouse report so the host cursor follows the finger.
 *
 * @param param Unused task argument.
 */
static void touch_mouse_task(void *param)
{
    ESP_LOGI(TAG, "Touch mouse task started");

    /* 0xffff marks "no previous position yet". */
    uint16_t prev_x = 0xffff;
    uint16_t prev_y = 0xffff;
    bool prev_pressed = false;

    while (1) {
        if (touch_read() == ESP_OK) {
            uint16_t x, y;
            bool pressed;
            get_coor(&x, &y, &pressed);

            /* Only emit movement while pressed and the host is listening. */
            if (pressed && is_usb_ready()) {
                if (prev_pressed && prev_x != 0xffff && prev_y != 0xffff) {
                    /* Relative movement since the last sample. */
                    int16_t delta_x = (int16_t)x - (int16_t)prev_x;
                    int16_t delta_y = (int16_t)y - (int16_t)prev_y;

                    send_hid_mouse_delta(delta_x, delta_y);
                    ESP_LOGI(TAG, "Mouse move: dX=%d, dY=%d", delta_x, delta_y);
                }
                prev_x = x;
                prev_y = y;
            } else if (!pressed) {
                /* Lift: forget the last position so the next touch starts fresh. */
                prev_x = 0xffff;
                prev_y = 0xffff;
            }
            prev_pressed = pressed;
        }

        /* 10 ms -> 100 Hz sampling. */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Brings up I2C, the touch panel and the USB HID device, then starts
 * the touch-to-mouse task.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Touch Mouse application");

    if (i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }

    if (touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch initialization failed");
        return;
    }

    if (usb_init() != ESP_OK) {
        ESP_LOGE(TAG, "USB initialization failed");
        return;
    }

    xTaskCreate(touch_mouse_task, "touch_mouse_task", 4096, NULL, 5, &touch_task_handle);
    if (touch_task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create touch mouse task");
        return;
    }

    ESP_LOGI(TAG, "Touch Mouse application started successfully");
}
