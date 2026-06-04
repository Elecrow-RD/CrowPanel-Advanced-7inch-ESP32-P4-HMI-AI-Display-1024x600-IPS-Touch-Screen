/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"   // Include the main header file containing required definitions and declarations
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
TaskHandle_t sd_task_handle;   // Task handle for the SD card test task
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
void sd_task(void *param)   // SD card test task function
{
    esp_err_t err = ESP_OK;   // Variable to store function return values (error codes)

    const char *file_hello = SD_MOUNT_POINT "/hello.txt";   // File path for SD card test file
    char *data = "hello world!";   // Data to be written into the file
    
    // Get SD card information
    get_sd_card_info();   // Print SD card info such as size, type, and speed
    
    while (1)   // Infinite loop to perform read/write test
    {
        // Write data to file
        err = write_string_file(file_hello, data);   // Write the "hello world!" string to the file
        if (err != ESP_OK)   // Check if writing failed
        {
            MAIN_ERROR("Write file failed");   // Print error message if writing fails
            continue;   // Continue to next iteration of loop
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);   // Delay 200ms to allow SD card to complete internal operations

        // Read data from file
        err = read_string_file(file_hello);   // Read the content from the written file
        if (err != ESP_OK)   // Check if reading failed
        {
            MAIN_ERROR("Read file failed");   // Print error message if reading fails
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);   // Delay 1 second before repeating the test
        MAIN_INFO("SD card test completed");   // Log message indicating test finished successfully
        vTaskDelete(NULL);   // Delete this task after finishing the test
    }
}

void init_fail(const char *name, esp_err_t err)   // Function to handle initialization failure
{
    while (1)   // Infinite loop to repeatedly print failure message
    {
        MAIN_ERROR("%s initialization failed [ %s ]", name, esp_err_to_name(err));   // Print module name and error description
        vTaskDelay(1000 / portTICK_PERIOD_MS);   // Delay 1 second before printing again
    }
}

void Init(void)   // System initialization function
{
    esp_err_t err = ESP_OK;   // Variable to store error codes
    
    // Initialize SD card
    err = sd_init();   // Call SD card initialization function
    if (err != ESP_OK)   // Check if initialization failed
        init_fail("SD card", err);   // Call error handling function if SD card initialization fails
}

void app_main(void)   // Main entry function of the application
{
    MAIN_INFO("----------SD card test program start----------\r\n");   // Print program start message
    
    // Initialize system
    Init();   // Call initialization function to set up SD card and other components
    
    // Create SD card test task
    xTaskCreatePinnedToCore(sd_task, "sd_task", 4096, NULL, 5, &sd_task_handle, 1);   // Create a FreeRTOS task to test SD card (core 1)
    
    MAIN_INFO("----------SD card test begin----------\r\n");   // Print message indicating test task has started
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
