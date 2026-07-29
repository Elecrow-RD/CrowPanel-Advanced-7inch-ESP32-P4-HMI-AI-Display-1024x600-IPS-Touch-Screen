/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "main.h"

/*---------------------------------------------------------------
 * Initialisation helpers
 *--------------------------------------------------------------*/

/**
 * @brief Halt forever if an init step failed.
 *
 * Logs the failing module name once and then loops so the user
 * can see which step is responsible from the serial output.
 *
 * @param name Module name.
 * @param err  Error code returned by the module.
 */
static void init_or_halt(const char *name, esp_err_t err)
{
    if (err != ESP_OK) {
        MAIN_ERROR("%s init failed: %s", name, esp_err_to_name(err));
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Main application entry point.
 *
 * Brings up the audio amplifier control, the I2S playback channel
 * and the PDM microphone, then records 5 seconds of audio and
 * plays it back over the speaker.
 */
void app_main(void)
{
    MAIN_INFO("Record 5s and playback original audio");

    /* Amplifier control GPIO must be ready before enabling audio. */
    esp_err_t err = audio_ctrl_init();
    init_or_halt("audio ctrl", err);
    /* Start with the amplifier off to avoid pop noise. */
    set_Audio_ctrl(false);

    /* I2S playback channel (16 kHz, 16-bit, stereo). */
    err = audio_init();
    init_or_halt("audio", err);

    /* PDM microphone input channel. */
    err = mic_init();
    init_or_halt("mic", err);

    /* Record 5 seconds and play it back immediately. */
    MAIN_INFO("Start 5s recording...");
    err = mic_read_to_audio(5);
    if (err != ESP_OK) {
        MAIN_ERROR("record/playback error: %s", esp_err_to_name(err));
    } else {
        MAIN_INFO("Playback done");
    }

    /* Nothing else to do; keep the task alive. */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
