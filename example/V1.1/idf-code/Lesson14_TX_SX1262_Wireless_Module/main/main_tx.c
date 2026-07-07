/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "include/main.h"

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
static lv_obj_t *s_hello_label = NULL;

static void lvgl_show_counter_label_init(void)
{
    if (lvgl_port_lock(0) != true) {
        MAIN_ERROR("LVGL lock failed");
        return;
    }

    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    s_hello_label = lv_label_create(screen);
    if (s_hello_label == NULL) {
        MAIN_ERROR("Create LVGL label failed");
        lvgl_port_unlock();
        return;
    }
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_42); 
    lv_style_set_text_color(&label_style, lv_color_black());
    lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);
    lv_obj_add_style(s_hello_label, &label_style, LV_PART_MAIN);

    lv_label_set_text(s_hello_label, "TX_Hello World:0");
    lv_obj_center(s_hello_label);

    lvgl_port_unlock();
}

static void ui_counter_task(void *param)
{
    char text[48];
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000); // 1 second = 1000ms
    
    for (;;) {
        uint32_t i = sx1262_get_tx_counter();
        int n = snprintf(text, sizeof(text), "TX_Hello World:%lu", (unsigned long)i);
        (void)n;

        if (lvgl_port_lock(0) == true) {
            if (s_hello_label != NULL) {
                lv_label_set_text(s_hello_label, text);
            }
            lvgl_port_unlock();
        }

        MAIN_INFO("TX msg: %s", text);
        
        // Use absolute time to ensure an exact one-second interval
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

static void init_or_halt(const char *name, esp_err_t err)
{
    static bool printed = false;
    while (err != ESP_OK) {
        if (!printed) {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));
            printed = true;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void Hardware_Init(void)
{
    esp_err_t err = ESP_OK;
    esp_ldo_channel_config_t ldo3_cfg = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cfg, &ldo3);
    init_or_halt("ldo3", err);

    esp_ldo_channel_config_t ldo4_cfg = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    err = esp_ldo_acquire_channel(&ldo4_cfg, &ldo4);
    init_or_halt("ldo4", err);

    // 1. Initialize LCD hardware and LVGL (important: must init before enabling backlight)
    err = display_init();
    if (err != ESP_OK) {  // Check error
        init_or_halt("LCD", err);  // Handle failure
    }
    MAIN_INFO("LCD init success");  // Print success log

    // 2. Turn on LCD backlight (brightness set to 100 = maximum)
    err = set_lcd_blight(100);  // Enable backlight
    if (err != ESP_OK) {  // Check error
        init_or_halt("LCD Backlight", err);  // Handle failure
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");  // Print success log

    // 3.wireless init
    err = sx1262_tx_init();
    if (err != ESP_OK) {  // Check error
        init_or_halt("Wireless Module init...", err);  // Handle failure
    }
    MAIN_INFO("The wireless module initialization was successful.");  // Print success log
}

static void lora_tx_task(void *param)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000); // 1 second = 1000ms
    
    while (1) {
        bool lora_tx_OK = false;
        lora_tx_OK = send_lora_pack_radio();
        if (lora_tx_OK != true) {
            MAIN_ERROR("LoRa TX failed");
        }
        
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

void app_main(void)
{
    MAIN_INFO("---------- LoRa TX ----------");
    Hardware_Init();

    lvgl_show_counter_label_init();
    MAIN_INFO("-------- LVGL Show OK ----------");

    // Create tasks and use the same priority to ensure synchronization
    xTaskCreatePinnedToCore(ui_counter_task, "ui_counter", 4096, NULL,
                                configMAX_PRIORITIES - 5, NULL, 0);

    xTaskCreatePinnedToCore(lora_tx_task, "sx1262_tx", 8192, NULL,
                                configMAX_PRIORITIES - 5, NULL, 1);

    MAIN_INFO("Tasks created, starting synchronized transmission...");
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
