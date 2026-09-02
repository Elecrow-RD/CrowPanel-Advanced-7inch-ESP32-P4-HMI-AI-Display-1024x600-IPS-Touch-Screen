#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp_extra.h"

/*---------------------------------------------------------------
 * LED blink task
 *--------------------------------------------------------------*/

/**
 * @brief FreeRTOS task that toggles the onboard LED every second.
 *
 * Initialises GPIO48 as an output first, then loops forever:
 * drive the pin high (LED on), wait one second, drive it low
 * (LED off), wait one second, and repeat.
 *
 * @param pvParameters Unused task parameter.
 */
void led_blink_task(void *pvParameters)
{
    /* Configure the LED pin as a push-pull output. */
    gpio_extra_init();

    while (1) {
        /* Turn the LED on and keep it lit for one second. */
        gpio_extra_set_level(1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        /* Turn the LED off and keep it dark for one second. */
        gpio_extra_set_level(0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Creates the LED blink task on the default core. The task stack
 * size is 2048 bytes and its priority is 5 (medium).
 */
void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 5, NULL);
}
