/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "include/main.h"

/*---------------------------------------------------------------
 * Power and state
 *--------------------------------------------------------------*/

static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

/* LVGL label that shows the TX counter on screen. */
static lv_obj_t *s_hello_label = NULL;

/*---------------------------------------------------------------
 * LVGL UI
 *--------------------------------------------------------------*/

/**
 * @brief Create the centred TX counter label.
 *
 * White background, black Montserrat-42 text showing
 * "NRF24_TX_Hello World:N", updated once per second.
 */
static void lvgl_show_counter_label_init(void)
{
    if (lvgl_port_lock(0) != true) {
        MAIN_ERROR("LVGL lock failed");
        return;
    }

    lv_obj_t *screen = lv_screen_active();
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

    lv_label_set_text(s_hello_label, "NRF24_TX_Hello World:0");
    lv_obj_center(s_hello_label);

    lvgl_port_unlock();
}

/*---------------------------------------------------------------
 * UI counter task
 *--------------------------------------------------------------*/

/**
 * @brief Update the on-screen TX counter once per second.
 * @param param Unused task argument.
 */
static void ui_counter_task(void *param)
{
    char text[48];
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000);

    for (;;) {
        uint32_t i = nrf24_get_tx_counter();
        int n = snprintf(text, sizeof(text), "NRF24_TX_Hello World:%lu", (unsigned long)i);
        (void)n;

        if (lvgl_port_lock(0) == true) {
            if (s_hello_label != NULL) {
                lv_label_set_text(s_hello_label, text);
            }
            lvgl_port_unlock();
        }

        /* Absolute delay keeps a precise 1 s cadence. */
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt and print the failing module name once, then loop.
 */
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

/**
 * @brief Bring up LDO power, LCD and the nRF24L01 transmitter.
 */
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

    err = display_init();
    if (err != ESP_OK) {
        init_or_halt("LCD", err);
    }
    MAIN_INFO("LCD init success");

    err = set_lcd_blight(100);
    if (err != ESP_OK) {
        init_or_halt("LCD Backlight", err);
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");

    err = nrf24_tx_init();
    if (err != ESP_OK) {
        init_or_halt("nRF24L01 Wireless Module init...", err);
    }
    MAIN_INFO("The nRF24L01 wireless module initialization was successful.");
}

/*---------------------------------------------------------------
 * nRF24 TX task
 *--------------------------------------------------------------*/

/**
 * @brief Increment the TX counter and send one packet per second.
 * @param param Unused task argument.
 */
static void nrf24_tx_task(void *param)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000);

    while (1) {
        nrf24_inc_tx_counter();
        bool nrf24_tx_OK = send_nrf24_pack_radio();
        if (nrf24_tx_OK != true) {
            MAIN_ERROR("nRF24L01 TX failed");
        }
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point (nRF24 TX side).
 */
void app_main(void)
{
    MAIN_INFO("---------- nRF24L01 TX ----------");
    Hardware_Init();

    lvgl_show_counter_label_init();
    MAIN_INFO("-------- LVGL Show OK ----------");

    xTaskCreatePinnedToCore(ui_counter_task, "ui_counter", 4096, NULL,
                           configMAX_PRIORITIES - 5, NULL, 0);

    xTaskCreatePinnedToCore(nrf24_tx_task, "nrf24_tx", 8192, NULL,
                           configMAX_PRIORITIES - 5, NULL, 1);

    MAIN_INFO("Tasks created, starting synchronized transmission...");
}
