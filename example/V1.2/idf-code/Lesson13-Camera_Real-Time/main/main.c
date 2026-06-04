/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"   // Include the main header file containing necessary definitions and declarations
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
static int video_node = -1;

static esp_ldo_channel_handle_t ldo4 = NULL;   // Handle for LDO channel 4 (used to control power output)
static esp_ldo_channel_handle_t ldo3 = NULL;   // Handle for LDO channel 3 (used to control power output)

// function declaration
void init_fail(const char *name, esp_err_t err);   // Function declaration for initialization failure handling
void Init(void);   // Function declaration for system initialization
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
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

    err = camera_video_init();
    if (err != ESP_OK)
        init_fail("camera", err);
    video_node = camera_work();
    if (-1 == video_node)
        init_fail("camera", ESP_FAIL);
}

void app_main(void)   // Main application entry point
{
    MAIN_INFO("----------Camera task----------\r\n");   // Print start log message

    Init();   // Call system initialization function

    set_camera_img_display(true);   // Enable camera image display

    MAIN_INFO("----------The screen is displaying.----------\r\n");   // Log that the screen is now displaying camera output
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
