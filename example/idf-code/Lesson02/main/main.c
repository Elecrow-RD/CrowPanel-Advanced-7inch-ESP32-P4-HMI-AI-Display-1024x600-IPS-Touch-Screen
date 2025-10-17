#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_extra.h"

void led_blink_task(void *pvParameters)
{
    // Initialize GPIO
    gpio_extra_init();

    while (1)
    {
        // LED is on
        gpio_extra_set_level(1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // LED is off
        gpio_extra_set_level(0);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // delay 1 second
    }
}

void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 5, NULL);
}
