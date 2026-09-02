/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "main.h"

/* File descriptor of the opened CSI video device (-1 = not opened). */
static int video_node = -1;

/* LDO channels that power the camera and LCD. */
static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt and print the failing module name once.
 * @param name Module name.
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
 * @brief Bring up LDO power, GPIO ISR service, LCD and camera.
 */
void Init(void)
{
    static esp_err_t err = ESP_OK;

    /* LDO3 = 2.5 V (IO domain), LDO4 = 3.3 V (camera/panel analogue). */
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

    /* GPIO ISR service is needed by some camera/video callbacks. */
    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        init_fail("gpio isr service", err);
    }

    /* LCD must be ready before we can draw camera frames. */
    err = display_init();
    if (err != ESP_OK) {
        init_fail("display", err);
    }

    err = set_lcd_blight(100);
    if (err != ESP_OK) {
        init_fail("LCD Backlight", err);
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");

    /* Camera: SCCB init + V4L2 open + stream start. */
    err = camera_video_init();
    if (err != ESP_OK) {
        init_fail("camera", err);
    }
    video_node = camera_work();
    if (-1 == video_node) {
        init_fail("camera", ESP_FAIL);
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Powers up the LCD and camera, then enables live camera display
 * on the screen.
 */
void app_main(void)
{
    MAIN_INFO("----------Camera task----------\r\n");

    Init();

    set_camera_img_display(true);

    MAIN_INFO("----------The screen is displaying.----------\r\n");
}
