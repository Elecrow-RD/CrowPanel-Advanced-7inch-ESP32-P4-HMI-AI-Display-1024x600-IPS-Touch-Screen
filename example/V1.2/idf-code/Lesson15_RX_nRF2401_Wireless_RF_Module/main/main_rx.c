/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "include/main.h"

/*---------------------------------------------------------------
 * Power and state
 *--------------------------------------------------------------*/

static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

/* LVGL label that shows the received packet counter. */
static lv_obj_t *s_rx_label = NULL;

/* Number of valid nRF24 packets received so far. */
static uint32_t rx_packet_count = 0;

/*---------------------------------------------------------------
 * nRF24 RX callback
 *--------------------------------------------------------------*/

/**
 * @brief Called by the wireless layer when an nRF24 packet is received.
 *
 * Increments the packet counter and updates the on-screen label.
 *
 * @param data Received payload.
 * @param len  Payload length.
 */
static void rx_data_callback(const char *data, size_t len)
{
    rx_packet_count++;

    if (lvgl_port_lock(0) == true) {
        if (s_rx_label != NULL) {
            char rx_text[64];
            snprintf(rx_text, sizeof(rx_text), "NRF24_RX_Hello World:%lu",
                     (unsigned long)rx_packet_count);
            lv_label_set_text(s_rx_label, rx_text);
        }
        lvgl_port_unlock();
    }

    char rx_display_text[64];
    snprintf(rx_display_text, sizeof(rx_display_text), "NRF24_RX_Hello World:%lu",
             (unsigned long)rx_packet_count);
    MAIN_INFO("NRF24 RX: %s", rx_display_text);
}

/*---------------------------------------------------------------
 * LVGL UI
 *--------------------------------------------------------------*/

/**
 * @brief Build the nRF24 RX screen: title and received message label.
 */
static void lvgl_show_rx_interface_init(void)
{
    if (lvgl_port_lock(0) != true) {
        MAIN_ERROR("LVGL lock failed");
        return;
    }

    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    /* Title at the top. */
    lv_obj_t *title_label = lv_label_create(screen);
    if (title_label != NULL) {
        lv_label_set_text(title_label, "nRF24L01 RX Receiver");
        static lv_style_t title_style;
        lv_style_init(&title_style);
        lv_style_set_text_font(&title_style, &lv_font_montserrat_42);
        lv_style_set_text_color(&title_style, lv_color_black());
        lv_style_set_bg_opa(&title_style, LV_OPA_TRANSP);
        lv_obj_add_style(title_label, &title_style, LV_PART_MAIN);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    }

    /* Received message label near centre. */
    s_rx_label = lv_label_create(screen);
    if (s_rx_label != NULL) {
        lv_label_set_text(s_rx_label, "NRF24_RX_Hello World:0");
        static lv_style_t rx_style;
        lv_style_init(&rx_style);
        lv_style_set_text_font(&rx_style, &lv_font_montserrat_42);
        lv_style_set_text_color(&rx_style, lv_color_black());
        lv_style_set_bg_opa(&rx_style, LV_OPA_TRANSP);
        lv_obj_add_style(s_rx_label, &rx_style, LV_PART_MAIN);
        lv_obj_align(s_rx_label, LV_ALIGN_CENTER, 0, -40);
    }

    lvgl_port_unlock();
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
 * @brief Bring up LDO power, LCD and the nRF24L01 receiver.
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

    err = nrf24_rx_init();
    if (err != ESP_OK) {
        init_or_halt("nRF24L01 Wireless Module RX init...", err);
    }
    MAIN_INFO("The nRF24L01 wireless module RX initialization was successful.");
}

/*---------------------------------------------------------------
 * nRF24 RX poll task
 *--------------------------------------------------------------*/

/**
 * @brief Poll the nRF24 receive path every 10 ms.
 *
 * nRF24 max payload is 32 bytes; pass that as the expected length.
 *
 * @param param Unused task argument.
 */
static void nrf24_rx_task(void *param)
{
    while (1) {
        received_nrf24_pack_radio(32);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point (nRF24 RX side).
 */
void app_main(void)
{
    MAIN_INFO("---------- nRF24L01 RX ----------");
    Hardware_Init();

    lvgl_show_rx_interface_init();
    MAIN_INFO("-------- LVGL RX Interface OK ----------");

    nrf24_set_rx_callback(rx_data_callback);
    MAIN_INFO("RX callback registered");

    xTaskCreatePinnedToCore(nrf24_rx_task, "nrf24_rx", 4096, NULL,
                           configMAX_PRIORITIES - 5, NULL, 1);

    MAIN_INFO("nRF24L01 RX receiver started, waiting for data...");
}
