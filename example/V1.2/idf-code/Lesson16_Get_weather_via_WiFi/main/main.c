/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include <errno.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_timer.h>
#include <sys/time.h>

#include "bsp_display.h"
#include "bsp_wifi.h"
#include "weather.h"

#define TAG "MAIN"
#define MAIN_INFO(fmt, ...)   ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define MAIN_DEBUG(fmt, ...)  ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define MAIN_ERROR(fmt, ...)  ESP_LOGE(TAG, fmt, ##__VA_ARGS__)

#define init_fail(fmt, ...)   ESP_LOGE(TAG, fmt":%d", ##__VA_ARGS__)

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Connect to WiFi, fetch weather data and show it on screen.
 *
 * Powers up LDO3, NVS, I2C, touch, display; connects to the
 * configured WiFi as a station; then loops fetching weather until a
 * valid response arrives. Once the weather and timestamp are
 * received, the system clock is set and the weather info, date and
 * weekday are drawn on top of a full-screen background image.
 */
void app_main(void)
{
    static esp_ldo_channel_handle_t ldo3 = NULL;
    esp_err_t err = ESP_OK;

    /* LDO3 powers the IO domain at 2.5 V. */
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK) {
        init_fail("ldo3", err);
    }

    /* NVS is required by the WiFi driver to store calibration. */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    err = i2c_init();
    if (err != ESP_OK) {
        init_fail("i2c", err);
    }

    err = touch_init();
    if (err != ESP_OK) {
        init_fail("display touch", err);
    }

    err = display_init();
    if (err != ESP_OK) {
        init_fail("display", err);
    }

    /* Bring up WiFi in STA mode and connect to the test AP. */
    bsp_wifi_init();
    bsp_wifi_sta_init();
    bsp_wifi_connect("yanfa_software", "yanfa-123456");

    /* Weather handle wraps the HTTP request + JSON parse. */
    weather_t *weather_handle = weather_create();
    char temp_text[32];
    double temp_c = 0.0;
    char weather_text[64];
    int timestamp = 0;
    char date_str[64];
    char week_str[64];

    /* Keep polling until WiFi is up and the first weather fetch succeeds. */
    while (1) {
        if (WIFI_CONNECTED == bsp_wifi_get_state()) {
            if (weather_get_weather(weather_handle, &temp_c, weather_text, &timestamp)) {
                snprintf(temp_text, sizeof(temp_text), "%.1lf C", temp_c);

                /* The API returns a Unix timestamp; set the system clock. */
                struct timeval tv = {
                    .tv_sec = 0,
                    .tv_usec = 0,
                };
                tv.tv_sec = timestamp;
                settimeofday(&tv, NULL);

                /* Format the local date and weekday. */
                time_t now = time(NULL);
                struct tm *local_time;
                local_time = localtime(&now);
                strftime(date_str, sizeof(date_str), "%Y/%m/%d", local_time);
                strftime(week_str, sizeof(week_str), "%A", local_time);
                ESP_LOGI(TAG, "time(NULL): %d", (int)time(NULL));
                break;
            }
        } else {
            MAIN_INFO("WIFI connecting......");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Full-screen background image declared in ui/image_both.c. */
    LV_IMG_DECLARE(image_both);

    lv_obj_t *ui_home = NULL;
    lv_obj_t *temperature_label_ = NULL;
    lv_obj_t *weather_label_ = NULL;
    lv_obj_t *date_label_ = NULL;
    lv_obj_t *week_label_ = NULL;

    if (lvgl_port_lock(0)) {
        /* Background image covering the whole screen. */
        ui_home = lv_img_create(lv_scr_act());
        lv_img_set_src(ui_home, &image_both);
        lv_obj_align(ui_home, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_size(ui_home, LV_HOR_RES, LV_VER_RES);
        lv_obj_clear_flag(ui_home, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM));
        lv_obj_set_style_bg_opa(ui_home, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_home, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(ui_home, LV_TEXT_ALIGN_RIGHT, 0);

        /* Temperature label, top-right, large font. */
        temperature_label_ = lv_label_create(ui_home);
        lv_obj_set_width(temperature_label_, LV_HOR_RES);
        lv_obj_set_height(temperature_label_, LV_SIZE_CONTENT);
        lv_obj_align(temperature_label_, LV_ALIGN_TOP_RIGHT, -50, 80);
        lv_label_set_text(temperature_label_, temp_text);
        lv_obj_set_style_text_font(temperature_label_, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(temperature_label_, lv_color_hex(0xFFFFFF), 0);

        /* Weather description below the temperature. */
        weather_label_ = lv_label_create(ui_home);
        lv_obj_set_width(weather_label_, LV_HOR_RES);
        lv_obj_set_height(weather_label_, LV_SIZE_CONTENT);
        lv_obj_align(weather_label_, LV_ALIGN_TOP_RIGHT, -50, 140);
        lv_label_set_text(weather_label_, weather_text);
        lv_obj_set_style_text_font(weather_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(weather_label_, lv_color_hex(0xFFFFFF), 0);

        /* Date below the weather. */
        date_label_ = lv_label_create(ui_home);
        lv_obj_set_width(date_label_, LV_HOR_RES);
        lv_obj_set_height(date_label_, LV_SIZE_CONTENT);
        lv_obj_align(date_label_, LV_ALIGN_TOP_RIGHT, -50, 180);
        lv_label_set_text(date_label_, date_str);
        lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(date_label_, lv_color_hex(0xFFFFFF), 0);

        /* Weekday below the date. */
        week_label_ = lv_label_create(ui_home);
        lv_obj_set_width(week_label_, LV_HOR_RES);
        lv_obj_set_height(week_label_, LV_SIZE_CONTENT);
        lv_obj_align(week_label_, LV_ALIGN_TOP_RIGHT, -50, 220);
        lv_label_set_text(week_label_, week_str);
        lv_obj_set_style_text_font(week_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(week_label_, lv_color_hex(0xFFFFFF), 0);

        lvgl_port_unlock();
    }

    /* Light the screen only after the first frame is ready. */
    set_lcd_blight(100);
}
