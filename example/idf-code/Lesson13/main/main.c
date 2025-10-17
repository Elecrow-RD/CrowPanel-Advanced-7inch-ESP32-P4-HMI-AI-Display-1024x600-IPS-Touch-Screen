/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"   // Include the main header file containing necessary definitions and declarations
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

TaskHandle_t lvgl_camera;   // Task handle for LVGL camera display task

static esp_ldo_channel_handle_t ldo4 = NULL;   // Handle for LDO channel 4 (used to control power output)
static esp_ldo_channel_handle_t ldo3 = NULL;   // Handle for LDO channel 3 (used to control power output)

// function declaration
void init_fail(const char *name, esp_err_t err);   // Function declaration for initialization failure handling
void Init(void);   // Function declaration for system initialization
void camera_display_task(void *param);   // Function declaration for camera display task
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

void camera_display_task(void *param)   // Task function to continuously refresh the camera display
{
    while (1)   // Infinite loop for periodic refreshing
    {
        // Directly refresh the camera display without the need for status check
        if (lvgl_port_lock(0))   // Lock LVGL port for safe access (timeout = 0)
        {
            camera_display_refresh();   // Refresh camera display content
            lvgl_port_unlock();   // Unlock LVGL port after refresh
        }
        vTaskDelay(23 / portTICK_PERIOD_MS);   // Delay approximately 23 ms between refreshes
    }
}

void init_fail(const char *name, esp_err_t err)   // Function to handle initialization failures
{
    static bool state = false;   // Flag to avoid repeated error logging
    while (1)   // Stay in infinite loop after failure
    {
        if (!state)   // Print error message only once
        {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));   // Log initialization failure with error name
            state = true;   // Update state to prevent repeated logs
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);   // Wait 1 second before looping again
    }
}

void Init(void)   // System initialization function
{
    static esp_err_t err = ESP_OK;   // Variable to store function return values

    esp_ldo_channel_config_t ldo3_cof = {   // LDO channel 3 configuration
        .chan_id = 3,   // Channel ID: 3
        .voltage_mv = 2500,   // Output voltage: 2.5V
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);   // Acquire and configure LDO3 channel
    if (err != ESP_OK)   // Check for error
        init_fail("ldo3", err);   // Handle initialization failure

    esp_ldo_channel_config_t ldo4_cof = {   // LDO channel 4 configuration
        .chan_id = 4,   // Channel ID: 4
        .voltage_mv = 3300,   // Output voltage: 3.3V
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);   // Acquire and configure LDO4 channel
    if (err != ESP_OK)   // Check for error
        init_fail("ldo4", err);   // Handle initialization failure

    err = gpio_install_isr_service(0);   // Install GPIO interrupt service routine
    if (err != ESP_OK)   // Check for error
        init_fail("gpio isr service", err);   // Handle initialization failure

    err = display_init();   // Initialize LCD display
    if (err != ESP_OK)   // Check for error
        init_fail("display", err);   // Handle initialization failure

    err = set_lcd_blight(100);  // Enable backlight with 100% brightness
    if (err != ESP_OK) {  // Check error
        init_fail("LCD Backlight", err);  // Handle failure
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");  // Log success message for backlight

    err = camera_init();   // Initialize camera module
    if (err != ESP_OK)   // Check for error
        init_fail("camera", err);   // Handle initialization failure

    camera_display();   // Initialize camera display output
}

void app_main(void)   // Main application entry point
{
    MAIN_INFO("----------Camera task----------\r\n");   // Print start log message

    Init();   // Call system initialization function

    camera_refresh();   // Refresh camera once before starting display loop

    xTaskCreatePinnedToCore(camera_display_task, "camera_display", 4096, NULL, configMAX_PRIORITIES - 4, &lvgl_camera, 1);
    // Create and start the camera display task on Core 1 with priority (max - 4)

    MAIN_INFO("----------The screen is displaying.----------\r\n");   // Log that the screen is now displaying camera output
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
