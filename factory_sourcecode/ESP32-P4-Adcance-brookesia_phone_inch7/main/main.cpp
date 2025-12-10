#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_heap_caps.h"
#include "esp_ldo_regulator.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"

#include "esp_brookesia.hpp"
#include "app_examples/phone/squareline/src/phone_app_squareline.hpp"
#include "apps.h"
#include "../components/espressif__esp32_p4_function_ev_board/elecrow_ui/include/elecrow_ui.h"
#include "../components/espressif__esp32_p4_function_ev_board/bsp_stc8h1kxx.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"

static const char *TAG = "main";

static TaskHandle_t battery_info_task_handle = NULL;     
uint32_t adc_voltage;
uint32_t bat_voltage;
uint32_t bat_level;
uint8_t bat_state;
uint8_t led_state;

/*
    The task to get battery information from stc8h1kxx via i2c
*/
void battery_info_task(void *param)
{
    while (1)
    {
        Battery_info_t battery_info = {0};
        stc8_battery_info_get(&battery_info);
        adc_voltage = battery_info.adc_voltage;
        bat_voltage = battery_info.bat_voltage;
        bat_level   = battery_info.bat_level;
        bat_state   = battery_info.bat_state;
        led_state   = battery_info.led_state;
        // ESP_LOGI(TAG, "adc_voltage = %lu mV", battery_info.adc_voltage);
        // ESP_LOGI(TAG, "bat_voltage = %lu mV", battery_info.bat_voltage);
        // ESP_LOGI(TAG, "bat_level = %d %%", battery_info.bat_level);
        // ESP_LOGI(TAG, "bat_state = %d", battery_info.bat_state);
        // ESP_LOGI(TAG, "led_state = %d", battery_info.led_state);
        if (battery_info.bat_voltage <= 3500) {
            ESP_LOGI(TAG, "esp_deep_sleep_start()");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            esp_deep_sleep_start();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

extern esp_lcd_touch_handle_t tp;
static int s_prev_brightness = 0;
static bool s_enter_sleep_flag = false;
void touch_detect_task(void *param)
{
    lv_indev_t *active_indev = lv_indev_get_act();  // Get the currently active input device
    while (1)
    {
        static uint32_t prev_boot_time_s = 0;
        uint32_t boot_time_s = esp_timer_get_time() / 1000 / 1000;

        uint16_t touch_x[1];
        uint16_t touch_y[1];
        uint8_t touch_cnt = 0;

        bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, NULL, &touch_cnt, 1);
        /*There are clicks on the touchscreen.*/
        if (touch_cnt) {
            prev_boot_time_s = boot_time_s;
            // If the screen was off before, restore the brightness
            if (s_enter_sleep_flag) {
                s_enter_sleep_flag = !s_enter_sleep_flag;
                bsp_display_brightness_set(s_prev_brightness);
            }
        }
        else {
            if (!s_enter_sleep_flag) {
                /* If there is no touch and it is in the non-screen-off state, and it 
                    has not been touched for more than a certain period of time, it enters the screen-off state*/
                if (60 < boot_time_s-prev_boot_time_s) {
                    s_enter_sleep_flag = !s_enter_sleep_flag;
                    s_prev_brightness = bsp_display_brightness_get();
                    bsp_display_brightness_set(0);
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_spiffs_mount());
    ESP_LOGI(TAG, "SPIFFS mount successfully");

#if CONFIG_EXAMPLE_ENABLE_SD_CARD
    esp_err_t sd_err = bsp_sdcard_mount();
    if (sd_err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_sdcard_mount failed: %s", esp_err_to_name(sd_err));
    }
    ESP_LOGI(TAG, "SD card mount successfully");
#endif

    ESP_ERROR_CHECK(bsp_extra_codec_init());

    stc8_i2c_init();
    Battery_info_t battery_info = {0};
    stc8_battery_info_get(&battery_info);
    ESP_LOGI(TAG, "adc_voltage = %lu mV", battery_info.adc_voltage);
    ESP_LOGI(TAG, "bat_voltage = %lu mV", battery_info.bat_voltage);
    ESP_LOGI(TAG, "bat_level = %d %%", battery_info.bat_level);
    ESP_LOGI(TAG, "bat_state = %d", battery_info.bat_state);
    ESP_LOGI(TAG, "led_state = %d", battery_info.led_state);
    if (battery_info.bat_voltage <= 3500) {
        ESP_LOGI(TAG, "esp_deep_sleep_start()");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }
    xTaskCreate(battery_info_task, "battery_info_task", 4096, NULL, 3, &battery_info_task_handle);

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = false,
        }
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    xTaskCreate(touch_detect_task, "touch_detect_task", 2048, NULL, 3, &battery_info_task_handle);

    bsp_display_lock(0);
    elecrow_screen();
    bsp_display_unlock();

    while (!elecrow_success)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    bsp_display_lock(0);

    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone();
    assert(phone != nullptr && "Failed to create phone");

    ESP_Brookesia_PhoneStylesheet_t *phone_stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_1024_600_DARK_STYLESHEET();
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone_stylesheet, "Create phone stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(*phone_stylesheet), "Add phone stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(*phone_stylesheet), "Activate phone stylesheet failed");

    assert(phone->begin() && "Failed to begin phone");

    PhoneAppSquareline *smart_gadget = new PhoneAppSquareline();
    assert(smart_gadget != nullptr && "Failed to create phone app squareline");
    assert((phone->installApp(smart_gadget) >= 0) && "Failed to install phone app squareline");

    Calculator *calculator = new Calculator();
    assert(calculator != nullptr && "Failed to create calculator");
    assert((phone->installApp(calculator) >= 0) && "Failed to begin calculator");

    MusicPlayer *music_player = new MusicPlayer();
    assert(music_player != nullptr && "Failed to create music_player");
    assert((phone->installApp(music_player) >= 0) && "Failed to begin music_player");

    AppSettings *app_settings = new AppSettings();
    assert(app_settings != nullptr && "Failed to create app_settings");
    assert((phone->installApp(app_settings) >= 0) && "Failed to begin app_settings");

    Game2048 *game_2048 = new Game2048();
    assert(game_2048 != nullptr && "Failed to create game_2048");
    assert((phone->installApp(game_2048) >= 0) && "Failed to begin game_2048");

    Camera *camera = new Camera(1024, 600);
    assert(camera != nullptr && "Failed to create camera");
    assert((phone->installApp(camera) >= 0) && "Failed to begin camera");

#if CONFIG_EXAMPLE_ENABLE_SD_CARD
    ESP_LOGW(TAG, "Using Video Player example requires inserting the SD card in advance and saving an MJPEG format video on the SD card");
    if (sd_err == ESP_OK) {
        AppVideoPlayer *app_video_player = new AppVideoPlayer();
        assert(app_video_player != nullptr && "Failed to create app_video_player");
        assert((phone->installApp(app_video_player) >= 0) && "Failed to begin app_video_player");
    }
#endif

    bsp_display_unlock();
}
