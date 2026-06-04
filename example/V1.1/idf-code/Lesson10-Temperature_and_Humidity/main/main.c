/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

TaskHandle_t read_dht20;
static lv_obj_t *dht20_data = NULL;
static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

void dht20_display()
{
    if (lvgl_port_lock(0))
    {
        dht20_data = lv_label_create(lv_scr_act()); /*Create a label object*/
        static lv_style_t label_style;
        lv_style_init(&label_style);                                                  /*Initialize a style*/
        lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);                             /*Set the style LVGL background color*/
        lv_obj_add_style(dht20_data, &label_style, LV_PART_MAIN);                     /*Add a style to an object*/
        lv_obj_set_style_text_color(dht20_data, LV_COLOR_WHITE, LV_PART_MAIN);        /*Set the style LVGL text color*/
        lv_obj_set_style_text_font(dht20_data, &lv_font_montserrat_30, LV_PART_MAIN); /*Set the style LVGL text font*/
        lv_obj_center(dht20_data);                                                    /*Align an object to the center on its parent*/
        lv_obj_set_style_bg_color(lv_scr_act(), LV_COLOR_BLACK, LV_PART_MAIN);        /*Set the screen's LVGL background color*/
        lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);            /*Set the screen's LVGL background transparency*/
        lv_label_set_text(dht20_data, "Temperature = 0.0 C  Humidity = 0.0 %%");      /*Set a new text for a label*/
        lvgl_port_unlock();
    }
}

void update_dht20_value(float temperature, float humidity)
{
    if (dht20_data)
    {
        char buffer[60];
        snprintf(buffer, sizeof(buffer), "Temperature = %.1f C  Humidity = %.1f %%", temperature, humidity); /*Format the data into a string*/
        lv_label_set_text(dht20_data, buffer);                                                               /*Set a new text for a label*/
    }
}

void dht20_read_task(void *param)
{
    static dht20_data_t measurements;
    while (1)
    {
        if (dht20_is_calibrated() == ESP_OK) /*The function for determining whether the DHT20 sensor is ready or not*/
        {
            MAIN_INFO("is calibrated....");
        }
        else
        {
            MAIN_INFO("is NOT calibrated....");
            if (dht20_begin() != ESP_OK) /*Reinitialize the DHT20 sensor*/
            {
                MAIN_ERROR("dht20 init again false");
                vTaskDelay(100 / portTICK_PERIOD_MS);
                continue;
            }
        }
        if (dht20_read_data(&measurements) != ESP_OK) /*Read the temperature and humidity data from the DHT20 sensor*/
        {
            if (lvgl_port_lock(0))
            {
                lv_label_set_text(dht20_data, "dht20 read data error"); /*Read failure message displayed*/
                lvgl_port_unlock();
            }
            MAIN_ERROR("dht20 read data error");
        }
        else /*Read successfully*/
        {
            if (lvgl_port_lock(0))
            {
                update_dht20_value(measurements.temperature, measurements.humidity); /*Update the DHT20 data displayed on the screen*/
                lvgl_port_unlock();
            }
            MAIN_INFO("Temperature:\t%.1fC", measurements.temperature);
            MAIN_INFO("Humidity:   \t%.1f%%", measurements.humidity);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void init_fail(const char *name, esp_err_t err)
{
    static bool state = false;
    while (1)
    {
        if (!state)
        {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));
            state = true;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Init(void)
{
    static esp_err_t err = ESP_OK;
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK)
        init_fail("ldo3", err);
    esp_ldo_channel_config_t ldo4_cof = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    if (err != ESP_OK)
        init_fail("ldo4", err);

    err = i2c_init(); /*I2C Initialization*/
    if (err != ESP_OK)
        init_fail("i2c", err);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    err = dht20_begin(); /*DHT20 Initialization*/
    if (err != ESP_OK)
        init_fail("dht20", err);

    err = display_init(); /*Display Initialization*/
    if (err != ESP_OK)
        init_fail("display", err);
}

void app_main(void)
{
    MAIN_INFO("----------Demo version----------");
    Init();
    set_lcd_blight(100);                                                                           /*Set the screen backlight to maximum brightness*/
    dht20_display();                                                                               /*Set the screen's LVGL default display page*/
    xTaskCreate(dht20_read_task, "read_dht20", 4096, NULL, configMAX_PRIORITIES - 5, &read_dht20); /*Create a DHT20 data display refresh thread*/
    MAIN_INFO("----------Start the test----------");
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/