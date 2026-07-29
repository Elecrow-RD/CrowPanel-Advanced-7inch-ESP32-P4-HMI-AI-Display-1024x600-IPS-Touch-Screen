#ifndef _BSP_AUDIO_H_
#define _BSP_AUDIO_H_

/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define AUDIO_TAG "AUDIO"

#define AUDIO_INFO(fmt, ...)   ESP_LOGI(AUDIO_TAG, fmt, ##__VA_ARGS__)
#define AUDIO_DEBUG(fmt, ...)  ESP_LOGD(AUDIO_TAG, fmt, ##__VA_ARGS__)
#define AUDIO_ERROR(fmt, ...) ESP_LOGE(AUDIO_TAG, fmt, ##__VA_ARGS__)

/* I2S1 audio pins: LRCLK=21, BCLK=22, SDATA=23, amplifier ctrl=30. */
#define AUDIO_GPIO_LRCLK 21
#define AUDIO_GPIO_BCLK  22
#define AUDIO_GPIO_SDATA 23
#define AUDIO_GPIO_CTRL  30

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the I2S playback channel (16 kHz, 16-bit, stereo).
 * @return ESP_OK on success.
 */
esp_err_t audio_init(void);

/**
 * @brief Configure the amplifier enable pin as a GPIO output.
 * @return ESP_OK on success.
 */
esp_err_t audio_ctrl_init(void);

/**
 * @brief Enable or disable the audio amplifier (active-low).
 * @param state true = on, false = off.
 * @return ESP_OK.
 */
esp_err_t set_Audio_ctrl(bool state);

/**
 * @brief Return the I2S transmit channel handle.
 */
i2s_chan_handle_t get_audio_handle(void);

#endif
