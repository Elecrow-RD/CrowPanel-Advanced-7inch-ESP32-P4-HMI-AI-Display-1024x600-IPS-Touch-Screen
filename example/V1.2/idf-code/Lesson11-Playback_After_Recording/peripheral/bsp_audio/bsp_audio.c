/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_audio.h"

/* I2S transmit channel handle used for playback. */
i2s_chan_handle_t tx_chan;

/*---------------------------------------------------------------
 * I2S playback channel
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the I2S playback channel.
 *
 * Configures I2S1 as master, 16 kHz sample rate, 16-bit stereo,
 * with BCLK/LRCLK/SDATA on the board audio pins.
 *
 * @return ESP_OK on success.
 */
esp_err_t audio_init(void)
{
    esp_err_t err = ESP_OK;

    /* I2S1 master, 6 DMA descriptors of 256 frames each. */
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_1,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 256,
        .auto_clear = true,
        .intr_priority = 0,
    };
    err = i2s_new_channel(&chan_cfg, &tx_chan, NULL);
    if (err != ESP_OK) {
        return err;
    }

    /* Standard I2S: 16 kHz, 16-bit, stereo. */
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 16000,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AUDIO_GPIO_BCLK,
            .ws = AUDIO_GPIO_LRCLK,
            .dout = AUDIO_GPIO_SDATA,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK) {
        return err;
    }

    err = i2s_channel_enable(tx_chan);
    return err;
}

/*---------------------------------------------------------------
 * Amplifier control
 *--------------------------------------------------------------*/

/**
 * @brief Configure the amplifier enable pin as a GPIO output.
 * @return ESP_OK on success.
 */
esp_err_t audio_ctrl_init(void)
{
    esp_err_t err = ESP_OK;

    const gpio_config_t audio_gpio_cofig = {
        .pin_bit_mask = 1ULL << AUDIO_GPIO_CTRL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&audio_gpio_cofig);
    return err;
}

/**
 * @brief Enable or disable the audio amplifier.
 *
 * The amplifier enable line is active-low, so the requested state is
 * inverted before being written to the GPIO.
 *
 * @param state true = amplifier on, false = off.
 * @return ESP_OK.
 */
esp_err_t set_Audio_ctrl(bool state)
{
    esp_err_t err = ESP_OK;
    bool status = !state;
    err = gpio_set_level(AUDIO_GPIO_CTRL, status);
    return err;
}

/**
 * @brief Return the I2S transmit channel handle.
 */
i2s_chan_handle_t get_audio_handle(void)
{
    return tx_chan;
}
