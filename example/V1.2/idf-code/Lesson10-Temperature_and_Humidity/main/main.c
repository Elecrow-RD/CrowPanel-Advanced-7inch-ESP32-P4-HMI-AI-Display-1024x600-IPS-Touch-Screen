/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "main.h"

/* Handle of the DHT20 reading task. */
TaskHandle_t read_dht20;

/* LVGL label that shows the temperature/humidity text. */
static lv_obj_t *dht20_data = NULL;

/* LDO channels that power the LCD and the IO domain. */
static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

/*---------------------------------------------------------------
 * LVGL UI
 *--------------------------------------------------------------*/

/**
 * @brief Create the label that displays the DHT20 readings.
 *
 * Black background, white Montserrat-30 text centred on screen,
 * initialised to zero values until the first measurement arrives.
 */
void dht20_display(void)
{
    if (lvgl_port_lock(0)) {
        dht20_data = lv_label_create(lv_screen_active());

        /* Transparent label background so the screen colour shows. */
        static lv_style_t label_style;
        lv_style_init(&label_style);
        lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);
        lv_obj_add_style(dht20_data, &label_style, LV_PART_MAIN);

        lv_obj_set_style_text_color(dht20_data, LV_COLOR_WHITE, LV_PART_MAIN);
        lv_obj_set_style_text_font(dht20_data, &lv_font_montserrat_30, LV_PART_MAIN);
        lv_obj_center(dht20_data);

        /* Black, fully opaque screen background. */
        lv_obj_set_style_bg_color(lv_screen_active(), LV_COLOR_BLACK, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);

        lv_label_set_text(dht20_data, "Temperature = 0.0 C  Humidity = 0.0 %%");
        lvgl_port_unlock();
    }
}

/**
 * @brief Update the on-screen label with the latest readings.
 * @param temperature Temperature in degrees Celsius.
 * @param humidity    Relative humidity in percent.
 */
void update_dht20_value(float temperature, float humidity)
{
    if (dht20_data) {
        char buffer[60];
        snprintf(buffer, sizeof(buffer),
                 "Temperature = %.1f C  Humidity = %.1f %%",
                 temperature, humidity);
        lv_label_set_text(dht20_data, buffer);
    }
}

/*---------------------------------------------------------------
 * DHT20 reading task
 *--------------------------------------------------------------*/

/**
 * @brief Periodically read the DHT20 and refresh the display.
 *
 * Checks calibration, reads the sensor every second, and updates
 * the LVGL label. If the sensor loses calibration it is re-initialised.
 *
 * @param param Unused task argument.
 */
void dht20_read_task(void *param)
{
    static dht20_data_t measurements;

    while (1) {
        /* Make sure the sensor is calibrated before reading. */
        if (dht20_is_calibrated() == ESP_OK) {
            MAIN_INFO("is calibrated....");
        } else {
            MAIN_INFO("is NOT calibrated....");
            if (dht20_begin() != ESP_OK) {
                MAIN_ERROR("dht20 init again false");
                vTaskDelay(100 / portTICK_PERIOD_MS);
                continue;
            }
        }

        /* Read temperature and humidity. */
        if (dht20_read_data(&measurements) != ESP_OK) {
            if (lvgl_port_lock(0)) {
                lv_label_set_text(dht20_data, "dht20 read data error");
                lvgl_port_unlock();
            }
            MAIN_ERROR("dht20 read data error");
        } else {
            if (lvgl_port_lock(0)) {
                update_dht20_value(measurements.temperature, measurements.humidity);
                lvgl_port_unlock();
            }
            MAIN_INFO("Temperature:\t%.1fC", measurements.temperature);
            MAIN_INFO("Humidity:   \t%.1f%%", measurements.humidity);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt and print the failing module name once, then sleep.
 * @param name Module name shown in the log.
 * @param err  Error code returned by the module.
 */
void init_fail(const char *name, esp_err_t err)
{
    static bool state = false;
    while (1) {
        if (!state) {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));
            state = true;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Bring up LDO power, I2C, DHT20 and the display.
 */
void Init(void)
{
    static esp_err_t err = ESP_OK;

    /* LDO3 = 2.5 V (IO domain), LDO4 = 3.3 V (panel analogue). */
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK) {
        init_fail("ldo3", err);
    }

    esp_ldo_channel_config_t ldo4_cof = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    if (err != ESP_OK) {
        init_fail("ldo4", err);
    }

    /* I2C must come up before the DHT20 sensor can be addressed. */
    err = i2c_init();
    if (err != ESP_OK) {
        init_fail("i2c", err);
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);

    err = dht20_begin();
    if (err != ESP_OK) {
        init_fail("dht20", err);
    }

    err = display_init();
    if (err != ESP_OK) {
        init_fail("display", err);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Powers up hardware, lights the screen, draws the label and starts
 * the DHT20 reading task.
 */
void app_main(void)
{
    MAIN_INFO("----------Demo version----------");
    Init();

    set_lcd_blight(100);
    dht20_display();

    xTaskCreate(dht20_read_task, "read_dht20", 4096, NULL,
                configMAX_PRIORITIES - 5, &read_dht20);
    MAIN_INFO("----------Start the test----------");
}
