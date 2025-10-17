// main.c
#include "main.h"

/* LDO channel handle */
static esp_ldo_channel_handle_t ldo3 = NULL;
static esp_ldo_channel_handle_t ldo4 = NULL;

/* Button callback function - turn on LED */
static void btn_on_click_event(lv_event_t *e)
{
    (void)e;
    gpio_extra_set_level(true);  // Turn on LED on GPIO48
    MAIN_INFO("LED turned ON");
}

/* Button callback function - turn off LED */
static void btn_off_click_event(lv_event_t *e)
{
    (void)e;
    gpio_extra_set_level(false);  // Turn off LED on GPIO48
    MAIN_INFO("LED turned OFF");
}

/* Create LED control UI */
static void create_led_control_ui(void)
{
    // Create main screen
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // Set white background

    // Create title label
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "LED Controller");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 50);
    // Font size
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    // Create LED ON button
    lv_obj_t *btn_on = lv_btn_create(scr);
    lv_obj_set_size(btn_on, 120, 50);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_event_cb(btn_on, btn_on_click_event, LV_EVENT_CLICKED, NULL);

    // ON button label
    lv_obj_t *label_on = lv_label_create(btn_on);
    lv_label_set_text(label_on, "LED ON");

    // Create LED OFF button
    lv_obj_t *btn_off = lv_btn_create(scr);
    lv_obj_set_size(btn_off, 120, 50);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn_off, btn_off_click_event, LV_EVENT_CLICKED, NULL);

    // OFF button label
    lv_obj_t *label_off = lv_label_create(btn_off);
    lv_label_set_text(label_off, "LED OFF");
}

/**
 • @brief Initialization failure handler (repeatedly prints error information)

 */
static void init_fail_handler(const char *module_name, esp_err_t err) {
    while (1) {  // Infinite loop to help debug which module failed to initialize
        MAIN_ERROR("[%s] init failed: %s", module_name, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 • @brief System initialization (LDO + LCD + backlight + other hardware)

 */
static void system_init(void) {
    esp_err_t err = ESP_OK;

    // 1. Initialize LDO (required for screen)
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK) init_fail_handler("ldo3", err);

    esp_ldo_channel_config_t ldo4_cof = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    if (err != ESP_OK) init_fail_handler("ldo4", err);
    MAIN_INFO("LDO3 and LDO4 init success");

    // 2. Initialize I2C (required for touch chip)
    MAIN_INFO("Initializing I2C...");
    err = i2c_init();
    if (err != ESP_OK) init_fail_handler("I2C", err);
    MAIN_INFO("I2C init success");

    // 3. Initialize touch panel (low-level driver)
    MAIN_INFO("Initializing touch panel...");
    err = touch_init();
    if (err != ESP_OK) init_fail_handler("Touch", err);
    MAIN_INFO("Touch panel init success");

    // 4. Initialize LCD hardware and LVGL (must initialize before turning on backlight)
    err = display_init();
    if (err != ESP_OK) init_fail_handler("LCD", err);
    MAIN_INFO("LCD init success");

    // 5. Turn on LCD backlight (brightness set to 100 = max)
    err = set_lcd_blight(100);
    if (err != ESP_OK) init_fail_handler("LCD Backlight", err);
    MAIN_INFO("LCD backlight opened (brightness: 100)");

    // 6. Initialize LED control GPIO (GPIO48)
    MAIN_INFO("Initializing GPIO48 for LED...");
    err = gpio_extra_init();
    if (err != ESP_OK) init_fail_handler("GPIO48", err);
    gpio_extra_set_level(false);  // Initially turn off LED
    MAIN_INFO("LED initialized to OFF state");

    // 7. Create UI
    create_led_control_ui();
    MAIN_INFO("UI created successfully");
}

void app_main(void)
{
    MAIN_INFO("Starting LED control application...");

    // System initialization (including LDO, LCD, touch, LED and all hardware)
    system_init();

    MAIN_INFO("System initialized successfully");

    while (1) {
        // Other background tasks can be placed here; maintain low power
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
