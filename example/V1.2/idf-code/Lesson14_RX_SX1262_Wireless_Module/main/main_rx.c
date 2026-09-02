/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "include/main.h"

/*---------------------------------------------------------------
 * Power and state
 *--------------------------------------------------------------*/

static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

/* LVGL labels: received message, RSSI and SNR. */
static lv_obj_t *s_rx_label = NULL;
static lv_obj_t *s_rssi_label = NULL;
static lv_obj_t *s_snr_label = NULL;

/* Number of valid LoRa packets received so far. */
static uint32_t rx_packet_count = 0;

/*---------------------------------------------------------------
 * LoRa RX callback
 *--------------------------------------------------------------*/

/**
 * @brief Called by the wireless layer when a LoRa packet is received.
 *
 * Increments the packet counter and updates the on-screen labels
 * (message, RSSI, SNR) under the LVGL lock, then logs the event.
 *
 * @param data Received payload.
 * @param len  Payload length.
 * @param rssi Received signal strength indicator in dBm.
 * @param snr  Signal-to-noise ratio in dB.
 */
static void rx_data_callback(const char *data, size_t len, float rssi, float snr)
{
    rx_packet_count++;

    if (lvgl_port_lock(0) == true) {
        if (s_rx_label != NULL) {
            char rx_text[64];
            snprintf(rx_text, sizeof(rx_text), "RX_Hello World:%lu",
                     (unsigned long)rx_packet_count);
            lv_label_set_text(s_rx_label, rx_text);
        }
        if (s_rssi_label != NULL) {
            char rssi_text[32];
            snprintf(rssi_text, sizeof(rssi_text), "RSSI: %.1f dBm", rssi);
            lv_label_set_text(s_rssi_label, rssi_text);
        }
        if (s_snr_label != NULL) {
            char snr_text[32];
            snprintf(snr_text, sizeof(snr_text), "SNR: %.1f dB", snr);
            lv_label_set_text(s_snr_label, snr_text);
        }
        lvgl_port_unlock();
    }

    char rx_display_text[64];
    snprintf(rx_display_text, sizeof(rx_display_text), "RX_Hello World:%lu",
             (unsigned long)rx_packet_count);
    MAIN_INFO("RX: %s (RSSI: %.1f dBm, SNR: %.1f dB)", rx_display_text, rssi, snr);
}

/*---------------------------------------------------------------
 * LVGL UI
 *--------------------------------------------------------------*/

/**
 * @brief Build the LoRa RX screen: title, message, RSSI and SNR labels.
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

    /* Shared style for the info labels. */
    static lv_style_t info_style;
    lv_style_init(&info_style);
    lv_style_set_text_font(&info_style, &lv_font_montserrat_42);
    lv_style_set_text_color(&info_style, lv_color_black());
    lv_style_set_bg_opa(&info_style, LV_OPA_TRANSP);

    /* Title at the top. */
    lv_obj_t *title_label = lv_label_create(screen);
    if (title_label != NULL) {
        lv_label_set_text(title_label, "LoRa RX Receiver");
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
        lv_label_set_text(s_rx_label, "RX_Hello World:0");
        static lv_style_t rx_style;
        lv_style_init(&rx_style);
        lv_style_set_text_font(&rx_style, &lv_font_montserrat_42);
        lv_style_set_text_color(&rx_style, lv_color_black());
        lv_style_set_bg_opa(&rx_style, LV_OPA_TRANSP);
        lv_obj_add_style(s_rx_label, &rx_style, LV_PART_MAIN);
        lv_obj_align(s_rx_label, LV_ALIGN_CENTER, 0, -40);
    }

    /* RSSI label bottom-left. */
    s_rssi_label = lv_label_create(screen);
    if (s_rssi_label != NULL) {
        lv_label_set_text(s_rssi_label, "RSSI: -- dBm");
        lv_obj_add_style(s_rssi_label, &info_style, LV_PART_MAIN);
        lv_obj_align(s_rssi_label, LV_ALIGN_CENTER, -180, 150);
    }

    /* SNR label bottom-right. */
    s_snr_label = lv_label_create(screen);
    if (s_snr_label != NULL) {
        lv_label_set_text(s_snr_label, "SNR: -- dB");
        lv_obj_add_style(s_snr_label, &info_style, LV_PART_MAIN);
        lv_obj_align(s_snr_label, LV_ALIGN_CENTER, 180, 150);
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
 * @brief Bring up LDO power, LCD and the SX1262 LoRa receiver.
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

    err = sx1262_rx_init();
    if (err != ESP_OK) {
        init_or_halt("Wireless Module RX init...", err);
    }
    MAIN_INFO("The wireless module RX initialization was successful.");
}

/*---------------------------------------------------------------
 * LoRa RX poll task
 *--------------------------------------------------------------*/

/**
 * @brief Poll the SX1262 receive flag every 10 ms.
 *
 * When a packet is signalled, reads its length and dispatches it
 * to the wireless layer for decoding.
 *
 * @param param Unused task argument.
 */
static void lora_rx_task(void *param)
{
    while (1) {
        if (sx1262_is_data_received()) {
            size_t len = sx1262_get_received_len();
            received_lora_pack_radio(len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point (RX side).
 *
 * Powers up hardware, draws the RX interface, registers the
 * receive callback and starts the RX poll task on core 1.
 */
void app_main(void)
{
    MAIN_INFO("---------- LoRa RX ----------");
    Hardware_Init();

    lvgl_show_rx_interface_init();
    MAIN_INFO("-------- LVGL RX Interface OK ----------");

    sx1262_set_rx_callback(rx_data_callback);
    MAIN_INFO("RX callback registered");

    xTaskCreatePinnedToCore(lora_rx_task, "sx1262_rx", 4096, NULL,
                           configMAX_PRIORITIES - 5, NULL, 1);

    MAIN_INFO("LoRa RX receiver started, waiting for data...");
}
