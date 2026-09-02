/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "main.h"

/* Handle of the SD card test task. */
TaskHandle_t sd_task_handle;

/*---------------------------------------------------------------
 * SD card read/write test task
 *--------------------------------------------------------------*/

/**
 * @brief Write a string to a file on the SD card, then read it back.
 *
 * Prints the SD card info first, then performs one write/read cycle
 * on /sdcard/hello.txt and deletes itself when done.
 *
 * @param param Unused task argument.
 */
void sd_task(void *param)
{
    esp_err_t err = ESP_OK;

    const char *file_hello = SD_MOUNT_POINT "/hello.txt";
    char *data = "hello world!";

    /* Show the card type, size and speed before using it. */
    get_sd_card_info();

    while (1) {
        /* Write the test string to the file. */
        err = write_string_file(file_hello, data);
        if (err != ESP_OK) {
            MAIN_ERROR("Write file failed");
            continue;
        }

        /* Let the card finish its internal write/flush. */
        vTaskDelay(200 / portTICK_PERIOD_MS);

        /* Read it back and log the content. */
        err = read_string_file(file_hello);
        if (err != ESP_OK) {
            MAIN_ERROR("Read file failed");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        MAIN_INFO("SD card test completed");

        /* One-shot test: the task is no longer needed afterwards. */
        vTaskDelete(NULL);
    }
}

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt and print the failing module name every second.
 * @param name Module name shown in the log.
 * @param err  Error code returned by the module.
 */
void init_fail(const char *name, esp_err_t err)
{
    while (1) {
        MAIN_ERROR("%s initialization failed [ %s ]", name, esp_err_to_name(err));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Initialise the SD card subsystem.
 */
void Init(void)
{
    esp_err_t err = ESP_OK;

    err = sd_init();
    if (err != ESP_OK) {
        init_fail("SD card", err);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Mounts the SD card and starts the test task pinned to core 1.
 */
void app_main(void)
{
    MAIN_INFO("----------SD card test program start----------\r\n");

    Init();

    /* Pin the SD task to core 1 so it does not starve the main task. */
    xTaskCreatePinnedToCore(sd_task, "sd_task", 4096, NULL, 5, &sd_task_handle, 1);

    MAIN_INFO("----------SD card test begin----------\r\n");
}
