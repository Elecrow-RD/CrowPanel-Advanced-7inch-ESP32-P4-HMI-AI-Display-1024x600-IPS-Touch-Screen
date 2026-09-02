/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_illuminate.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define MAIN_TAG "MAIN"
#define MAIN_INFO(fmt, ...)   ESP_LOGI(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_ERROR(fmt, ...)  ESP_LOGE(MAIN_TAG, fmt, ##__VA_ARGS__)

/* LDO channels that power the LCD and the IO domain. */
static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

/*---------------------------------------------------------------
 * LVGL UI
 *--------------------------------------------------------------*/

/**
 * @brief Draw the "Hello Elecrow" label centred on the screen.
 *
 * Must be called after display_init(). Locks the LVGL port so the
 * draw calls are thread-safe, then creates a white screen with a
 * black 42-pt label in the middle.
 */
static void lvgl_show_hello_elecrow(void)
{
    /* Acquire the LVGL lock before touching any LVGL object. */
    if (lvgl_port_lock(0) != true) {
        MAIN_ERROR("LVGL lock failed");
        return;
    }

    /* White, fully opaque background so the text is readable. */
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* Label that will hold the greeting text. */
    lv_obj_t *hello_label = lv_label_create(screen);
    if (hello_label == NULL) {
        MAIN_ERROR("Create LVGL label failed");
        lvgl_port_unlock();
        return;
    }
    lv_label_set_text(hello_label, "Hello Elecrow");

    /* Montserrat 42, black text, transparent label background. */
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_42);
    lv_style_set_text_color(&label_style, LV_COLOR_BLACK);
    lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);
    lv_obj_add_style(hello_label, &label_style, LV_PART_MAIN);

    /* Centre the label on the screen. */
    lv_obj_center(hello_label);

    /* Release the lock so the LVGL task can render the frame. */
    lvgl_port_unlock();
}

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt forever and print the failure reason every second.
 *
 * Used when a critical init step fails so the user can see which
 * module is responsible from the log output.
 *
 * @param module_name Name of the failing module.
 * @param err         Error code returned by the module.
 */
static void init_fail_handler(const char *module_name, esp_err_t err)
{
    while (1) {
        MAIN_ERROR("[%s] init failed: %s", module_name, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Bring up the power, LCD and backlight.
 *
 * Order matters: LDO3/LDO4 power the panel and IO domain, then the
 * LCD + LVGL are initialised, and only after that is the backlight
 * enabled so the user never sees a half-initialised panel lit up.
 */
static void system_init(void)
{
    esp_err_t err = ESP_OK;
    MAIN_INFO("Power/display initialization started");

    /* LDO3 = 2.5 V (IO domain), LDO4 = 3.3 V (panel analogue). */
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    MAIN_INFO("Acquiring LDO3");
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK) {
        init_fail_handler("ldo3", err);
    }
    MAIN_INFO("LDO3 ready");

    esp_ldo_channel_config_t ldo4_cof = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    MAIN_INFO("Acquiring LDO4");
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    if (err != ESP_OK) {
        init_fail_handler("ldo4", err);
    }
    MAIN_INFO("LDO4 ready");

    /* LCD + LVGL must be ready before the backlight is turned on. */
    MAIN_INFO("Initializing MIPI display and LVGL");
    err = display_init();
    if (err != ESP_OK) {
        init_fail_handler("LCD", err);
    }
    MAIN_INFO("LCD init success");

    /* Full brightness (0-100 scale). */
    err = set_lcd_blight(100);
    if (err != ESP_OK) {
        init_fail_handler("LCD Backlight", err);
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Powers up the LCD, lights the backlight, then draws the greeting
 * text. After this returns the ESP-IDF main task ends; the LVGL
 * task created by lvgl_port_init keeps rendering in the background.
 */
void app_main(void)
{
    MAIN_INFO("Start Hello Elecrow Display Demo");

    system_init();

    lvgl_show_hello_elecrow();
    MAIN_INFO("Show 'Hello Elecrow' success");
}
