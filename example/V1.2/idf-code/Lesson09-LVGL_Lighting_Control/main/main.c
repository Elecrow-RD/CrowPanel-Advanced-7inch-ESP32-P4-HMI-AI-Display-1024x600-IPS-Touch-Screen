#include "main.h"

/*---------------------------------------------------------------
 * Power / handle storage
 *--------------------------------------------------------------*/

/* LDO channels that power the LCD and the IO domain. */
static esp_ldo_channel_handle_t ldo3 = NULL;
static esp_ldo_channel_handle_t ldo4 = NULL;

/*---------------------------------------------------------------
 * LVGL UI callbacks
 *--------------------------------------------------------------*/

/**
 * @brief Button click callback - turn the LED on.
 *
 * Triggered by LVGL when the "LED ON" button is clicked. Drives
 * GPIO48 high and logs the action.
 *
 * @param e LVGL event object (unused).
 */
static void btn_on_click_event(lv_event_t *e)
{
    (void)e;
    gpio_extra_set_level(true);
    MAIN_INFO("LED turned ON");
}

/**
 * @brief Button click callback - turn the LED off.
 * @param e LVGL event object (unused).
 */
static void btn_off_click_event(lv_event_t *e)
{
    (void)e;
    gpio_extra_set_level(false);
    MAIN_INFO("LED turned OFF");
}

/*---------------------------------------------------------------
 * LVGL UI construction
 *--------------------------------------------------------------*/

/**
 * @brief Build the LED control screen: title + ON/OFF buttons.
 *
 * White background, a centred title and two buttons stacked
 * vertically. Each button has a label and a click event that
 * drives GPIO48.
 */
static void create_led_control_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    /* Title label at the top. */
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "LED Controller");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    /* ON button above centre. */
    lv_obj_t *btn_on = lv_button_create(scr);
    lv_obj_set_size(btn_on, 120, 50);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_event_cb(btn_on, btn_on_click_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_on = lv_label_create(btn_on);
    lv_label_set_text(label_on, "LED ON");

    /* OFF button below centre. */
    lv_obj_t *btn_off = lv_button_create(scr);
    lv_obj_set_size(btn_off, 120, 50);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn_off, btn_off_click_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label_off = lv_label_create(btn_off);
    lv_label_set_text(label_off, "LED OFF");
}

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt forever and print the failing module name every second.
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
 * @brief Bring up LDO power, I2C, touch, LCD, backlight and LED GPIO.
 *
 * Order matters: LDO3/LDO4 power the panel; I2C must exist before
 * the touch driver; the LCD + LVGL must be ready before the
 * backlight is enabled; finally the LED GPIO is configured off.
 */
static void system_init(void)
{
    esp_err_t err = ESP_OK;

    /* LDO3 = 2.5 V (IO domain), LDO4 = 3.3 V (panel analogue). */
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK) {
        init_fail_handler("ldo3", err);
    }

    esp_ldo_channel_config_t ldo4_cof = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    if (err != ESP_OK) {
        init_fail_handler("ldo4", err);
    }
    MAIN_INFO("LDO3 and LDO4 init success");

    /* I2C is required by the GT911 touch controller. */
    MAIN_INFO("Initializing I2C...");
    err = i2c_init();
    if (err != ESP_OK) {
        init_fail_handler("I2C", err);
    }
    MAIN_INFO("I2C init success");

    /* Touch panel (low-level GT911 driver). */
    MAIN_INFO("Initializing touch panel...");
    err = touch_init();
    if (err != ESP_OK) {
        init_fail_handler("Touch", err);
    }
    MAIN_INFO("Touch panel init success");

    /* LCD + LVGL must be ready before the backlight is turned on. */
    err = display_init();
    if (err != ESP_OK) {
        init_fail_handler("LCD", err);
    }
    MAIN_INFO("LCD init success");

    /* Full brightness. */
    err = set_lcd_blight(100);
    if (err != ESP_OK) {
        init_fail_handler("LCD Backlight", err);
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");

    /* LED GPIO starts off so the UI reflects the initial state. */
    MAIN_INFO("Initializing GPIO48 for LED...");
    err = gpio_extra_init();
    if (err != ESP_OK) {
        init_fail_handler("GPIO48", err);
    }
    gpio_extra_set_level(false);
    MAIN_INFO("LED initialized to OFF state");

    /* Build the on-screen buttons. */
    create_led_control_ui();
    MAIN_INFO("UI created successfully");
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Powers up all hardware, lights the screen and builds the LED
 * control UI. After this returns the LVGL task keeps rendering
 * and dispatching touch events in the background.
 */
void app_main(void)
{
    MAIN_INFO("Starting LED control application...");

    system_init();

    MAIN_INFO("System initialized successfully");

    while (1) {
        /* The main task has nothing else to do; let LVGL run. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
