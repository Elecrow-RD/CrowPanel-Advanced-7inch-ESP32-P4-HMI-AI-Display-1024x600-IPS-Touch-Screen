/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_illuminate.h"  // Include LCD initialization and backlight control interface
#include "lvgl.h"         // Include LVGL graphics library API
#include "freertos/FreeRTOS.h"  // Include FreeRTOS core header
#include "freertos/task.h"      // Include FreeRTOS task API
#include "esp_ldo_regulator.h"  // Include LDO (Low Dropout Regulator) API
#include "esp_log.h"       // Include LOG printing interface

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Macro definition—————————————————————————————————————————*/
#define MAIN_TAG "MAIN"  // Define log tag for this module
#define MAIN_INFO(fmt, ...) ESP_LOGI(MAIN_TAG, fmt, ##__VA_ARGS__)   // Info level log macro
#define MAIN_ERROR(fmt, ...) ESP_LOGE(MAIN_TAG, fmt, ##__VA_ARGS__)  // Error level log macro
/*———————————————————————————————————————Macro definition end———————————————————————————————————————*/

static esp_ldo_channel_handle_t ldo4 = NULL;  // Handle for LDO channel 4
static esp_ldo_channel_handle_t ldo3 = NULL;  // Handle for LDO channel 3

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
/**
 * @brief LVGL text display function (display "Hello Elecrow")
 */
static void lvgl_show_hello_elecrow(void) {
    // 1. Lock LVGL: ensure thread-safe operations
    if (lvgl_port_lock(0) != true) {  // 0 means non-blocking wait for the lock (timeout = 0)
        MAIN_ERROR("LVGL lock failed");  // Print error if lock fails
        return;  // Exit function
    }

    // 2. Create screen background (optional: set background color for better text visibility)
    lv_obj_t *screen = lv_scr_act();  // Get current active screen object
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);  // Set background color to white
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);      // Set background fully opaque

    // 3. Create label object (parent object = current screen)
    lv_obj_t *hello_label = lv_label_create(screen);  // Create label
    if (hello_label == NULL) {  // Check if creation failed
        MAIN_ERROR("Create LVGL label failed");  // Log error
        lvgl_port_unlock();  // Unlock LVGL before returning
        return;  // Exit function
    }

    // 4. Set label text content
    lv_label_set_text(hello_label, "Hello Elecrow");  // Set label text

    // 5. Configure label style (font, color, background)
    static lv_style_t label_style;  // Define a style object
    lv_style_init(&label_style);    // Initialize style object
    // Set font: Montserrat size 42 (must be enabled in LVGL config)
    lv_style_set_text_font(&label_style, &lv_font_montserrat_42);
    // Set text color to black (contrast with white background)
    lv_style_set_text_color(&label_style, LV_COLOR_BLACK);
    // Set label background transparent (avoid blocking screen background)
    lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);
    // Apply style to the label
    lv_obj_add_style(hello_label, &label_style, LV_PART_MAIN);

    // 6. Adjust label position: center on screen
    lv_obj_center(hello_label);

    // 7. Unlock LVGL: release lock, allow LVGL task to render
    lvgl_port_unlock();
}

/**
 * @brief Initialization failure handler (print error message repeatedly)
 */
static void init_fail_handler(const char *module_name, esp_err_t err) {
    while (1) {  // Infinite loop
        MAIN_ERROR("[%s] init failed: %s", module_name, esp_err_to_name(err));  // Print error with module name
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay 1 second between logs
    }
}

/**
 * @brief System initialization (LCD + Backlight)
 */
static void system_init(void) {
    esp_err_t err = ESP_OK;  // Error variable initialized to OK
    esp_ldo_channel_config_t ldo3_cof = {  // LDO3 configuration
        .chan_id = 3,         // LDO channel ID = 3
        .voltage_mv = 2500,   // Set output voltage = 2500 mV
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);  // Acquire LDO3 channel
    if (err != ESP_OK)  // Check error
        init_fail_handler("ldo3", err);  // Handle failure
    esp_ldo_channel_config_t ldo4_cof = {  // LDO4 configuration
        .chan_id = 4,         // LDO channel ID = 4
        .voltage_mv = 3300,   // Set output voltage = 3300 mV
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);  // Acquire LDO4 channel
    if (err != ESP_OK)  // Check error
        init_fail_handler("ldo4", err);  // Handle failure

    // 1. Initialize LCD hardware and LVGL (important: must init before enabling backlight)
    err = display_init();
    if (err != ESP_OK) {  // Check error
        init_fail_handler("LCD", err);  // Handle failure
    }
    MAIN_INFO("LCD init success");  // Print success log

    // 2. Turn on LCD backlight (brightness set to 100 = maximum)
    err = set_lcd_blight(100);  // Enable backlight
    if (err != ESP_OK) {  // Check error
        init_fail_handler("LCD Backlight", err);  // Handle failure
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");  // Print success log
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/

/*——————————————————————————————————————————Main function—————————————————————————————————————————*/
void app_main(void) {
    MAIN_INFO("Start Hello Elecrow Display Demo");  // Print start log

    // 1. System initialization (LCD + Backlight)
    system_init();
    // 2. Show "Hello Elecrow" text
    lvgl_show_hello_elecrow();
    MAIN_INFO("Show 'Hello Elecrow' success");  // Print success log

}
/*———————————————————————————————————————Main function end———————————————————————————————————————*/
