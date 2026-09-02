#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Executed automatically by the ESP-IDF startup code after the system
 * boots. Runs as the default FreeRTOS main task. Prints an incrementing
 * counter to the serial console once per second so the learner can
 * verify that the toolchain, target chip and serial console are all
 * configured correctly.
 */
void app_main(void)
{
    int i = 0;

    /* Repeat forever: the loop body prints one line then yields the
     * CPU for one second so other FreeRTOS tasks can still run. */
    while (1) {
        printf("Hello world: %d\n", i++);

        /* Convert 1000 ms into ticks before delaying this task. */
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
