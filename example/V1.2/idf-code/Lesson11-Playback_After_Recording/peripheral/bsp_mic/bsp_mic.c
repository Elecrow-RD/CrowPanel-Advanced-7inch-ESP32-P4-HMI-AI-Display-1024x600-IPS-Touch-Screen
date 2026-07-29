/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_mic.h"

/* I2S receive channel handle for the PDM microphone. */
i2s_chan_handle_t rx_chan;

/*---------------------------------------------------------------
 * Microphone initialisation
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the PDM microphone on I2S0.
 *
 * Configures a PDM RX channel at 16 kHz, mono, 16-bit, with the
 * high-pass filter enabled to remove DC offset.
 *
 * @return ESP_OK on success.
 */
esp_err_t mic_init(void)
{
    esp_err_t err = ESP_OK;

    /* I2S0 master, 6 DMA descriptors of 256 frames. */
    i2s_chan_config_t rx_chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 256,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = true,
        .allow_pd = false,
        .intr_priority = 0,
    };
    err = i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan);
    if (err != ESP_OK) {
        return err;
    }

    /* PDM RX: 16 kHz mono with 8x down-sampling and HP filter. */
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = {
            .sample_rate_hz = MIC_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            .dn_sample_mode = I2S_PDM_DSR_8S,
            .bclk_div = 8,
        },
        /* PDM data width is fixed at 16 bits. */
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_PDM_SLOT_LEFT,
            .hp_en = true,
            .hp_cut_off_freq_hz = 35.5,
            .amplify_num = 1,
        },
        .gpio_cfg = {
            .clk = MIC_GPIO_CLK,
            .din = MIC_GPIO_SDIN2,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };
    err = i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg);
    if (err != ESP_OK) {
        return err;
    }

    err = i2s_channel_enable(rx_chan);
    return err;
}

/*---------------------------------------------------------------
 * Record and playback
 *--------------------------------------------------------------*/

/**
 * @brief Record a number of seconds from the mic and play it back.
 *
 * Allocates SPIRAM buffers for the recording and the stereo playback
 * stream, reads the requested duration from the mic, amplifies and
 * clamps each sample, duplicates mono into stereo, then writes the
 * result to the I2S playback channel.
 *
 * @param rec_seconds Recording duration in seconds (max 60).
 * @return ESP_OK on success.
 */
esp_err_t mic_read_to_audio(size_t rec_seconds)
{
    esp_err_t err = ESP_OK;
    size_t bytes_read = 0;
    size_t bytes_write = 0;

    /* Guard against unreasonably long recordings. */
    if (rec_seconds > 60) {
        MIC_INFO("Exceeding the maximum recording duration");
        return ESP_FAIL;
    }

    size_t rec_size = rec_seconds * BYTE_RATE;
    i2s_chan_handle_t write_handle = get_audio_handle();

    /* Mono recording buffer in SPIRAM. */
    int16_t *read_buf = heap_caps_malloc(rec_size, MALLOC_CAP_SPIRAM);
    if (NULL == read_buf) {
        MIC_INFO("mic read_buf fail to apply");
        return ESP_FAIL;
    }
    memset(read_buf, 0, rec_size);

    /* Stereo playback buffer is twice the mono size. */
    int16_t *write_buf = heap_caps_malloc(rec_size * 2, MALLOC_CAP_SPIRAM);
    if (NULL == write_buf) {
        MIC_INFO("mic write_buf fail to apply");
        return ESP_FAIL;
    }
    memset(write_buf, 0, rec_size * 2);

    /* Read the whole recording in one I2S call. */
    MIC_INFO("Start Recording %d of audio data", rec_seconds);
    err = i2s_channel_read(rx_chan, read_buf, rec_size, &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) {
        MIC_INFO("read mic data fail");
        return err;
    }
    if (bytes_read != rec_size) {
        MIC_INFO("read mic data num error");
        return err;
    }

    /* Amplify 10x and clamp to int16 range; copy mono to both channels. */
    int32_t data;
    int32_t array_size = rec_size / 2;
    for (size_t i = 0, j = 0; i < array_size; i++, j += 2) {
        data = read_buf[i] * 10;
        if (data > 32767) {
            data = 32767;
        } else if (data < -32768) {
            data = -32768;
        }
        write_buf[j] = data;
        write_buf[j + 1] = data;
    }

    /* Play back through the amplifier. */
    MIC_INFO("Start play audio data");
    set_Audio_ctrl(true);
    err = i2s_channel_write(write_handle, write_buf, rec_size * 2, &bytes_write, portMAX_DELAY);
    if (err != ESP_OK) {
        set_Audio_ctrl(false);
        heap_caps_free(read_buf);
        heap_caps_free(write_buf);
        return err;
    }

    set_Audio_ctrl(false);
    heap_caps_free(read_buf);
    heap_caps_free(write_buf);
    return err;
}
