/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "main.h"

/* LDO channels that power the SD card and audio domain. */
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
 * @brief Bring up LDO power, SD card, audio amplifier and I2S playback.
 */
void Init(void)
{
    static esp_err_t err = ESP_OK;

    /* LDO3 = 2.5 V (IO domain), LDO4 = 3.3 V (SD/audio analogue). */
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

    /* SD card must be mounted before the WAV file can be opened. */
    err = sd_init();
    if (err != ESP_OK) {
        init_fail("sd", err);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    /* Amplifier control GPIO; start muted. */
    err = audio_ctrl_init();
    if (err != ESP_OK) {
        init_fail("audio ctrl", err);
    }
    set_Audio_ctrl(false);

    /* I2S playback channel. */
    err = audio_init();
    if (err != ESP_OK) {
        init_fail("audio", err);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Mounts the SD card, brings up audio, then plays the WAV file
 * /sdcard/huahai.wav through the speaker.
 */
void app_main(void)
{
    MAIN_INFO("----------Demo version----------");
    MAIN_INFO("----------Start the test--------");
    Init();

    Audio_play_wav_sd("/sdcard/huahai.wav");
}
