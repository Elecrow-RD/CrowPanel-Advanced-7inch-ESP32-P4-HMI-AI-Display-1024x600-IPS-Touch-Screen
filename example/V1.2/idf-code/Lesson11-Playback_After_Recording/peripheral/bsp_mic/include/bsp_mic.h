#ifndef _BSP_MIC_H_
#define _BSP_MIC_H_

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
#include "driver/i2s_pdm.h"
#include "bsp_audio.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define MIC_TAG "MIC"

#define MIC_INFO(fmt, ...)   ESP_LOGI(MIC_TAG, fmt, ##__VA_ARGS__)
#define MIC_DEBUG(fmt, ...)  ESP_LOGD(MIC_TAG, fmt, ##__VA_ARGS__)
#define MIC_ERROR(fmt, ...) ESP_LOGE(MIC_TAG, fmt, ##__VA_ARGS__)

/* PDM mic: clock on GPIO24, data on GPIO26. */
#define MIC_GPIO_CLK   (24)
#define MIC_GPIO_SDIN2 (26)

/* 16 kHz mono, 16-bit -> 32000 bytes per second. */
#define MIC_SAMPLE_RATE 16000
#define BYTE_RATE       ((16000 * (16 / 8)) * 1)

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the PDM microphone on I2S0.
 * @return ESP_OK on success.
 */
esp_err_t mic_init(void);

/**
 * @brief Record rec_seconds from the mic and play it back.
 * @param rec_seconds Recording duration (max 60 s).
 * @return ESP_OK on success.
 */
esp_err_t mic_read_to_audio(size_t rec_seconds);

#endif
