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
 */
esp_err_t audio_ctrl_init(void)
{
    esp_err_t err = ESP_OK;
    const gpio_config_t gpio_cofig = {
        .pin_bit_mask = 1ULL << AUDIO_GPIO_CTRL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&gpio_cofig);
    return err;
}

/**
 * @brief Enable or disable the audio amplifier (active-low).
 * @param state true = on, false = off.
 */
esp_err_t set_Audio_ctrl(bool state)
{
    esp_err_t err = ESP_OK;
    bool status = !state;
    err = gpio_set_level(AUDIO_GPIO_CTRL, status);
    return err;
}

/*---------------------------------------------------------------
 * WAV playback
 *--------------------------------------------------------------*/

/**
 * @brief Validate the 44-byte WAV header of a file.
 *
 * Checks the RIFF/WAVE/fmt /data markers, PCM format, supported
 * channel counts, sample rates and bit depths. Restores the file
 * position on return.
 *
 * @param file Opened WAV file.
 * @return true if the header is valid PCM WAV.
 */
bool validate_wav_header(FILE *file)
{
    if (file == NULL) {
        AUDIO_ERROR("File pointer is NULL");
        return false;
    }

    /* Remember where we were so validation does not disturb playback. */
    long original_position = ftell(file);
    if (original_position == -1) {
        AUDIO_ERROR("Cannot get current file position");
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        AUDIO_ERROR("Cannot seek to file beginning");
        return false;
    }

    /* The canonical WAV header is 44 bytes. */
    uint8_t header[44];
    size_t bytes_read = fread(header, 1, 44, file);
    if (bytes_read != 44) {
        AUDIO_ERROR("Cannot read complete WAV header (%d bytes)", bytes_read);
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    /* RIFF chunk descriptor. */
    if (memcmp(header, "RIFF", 4) != 0) {
        AUDIO_ERROR("Invalid RIFF header");
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    if (memcmp(header + 8, "WAVE", 4) != 0) {
        AUDIO_ERROR("Invalid WAVE format");
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    /* fmt subchunk. */
    if (memcmp(header + 12, "fmt ", 4) != 0) {
        AUDIO_ERROR("Invalid fmt subchunk");
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    /* Only linear PCM (format = 1) is supported. */
    uint16_t audio_format = *(uint16_t *)(header + 20);
    if (audio_format != 1) {
        AUDIO_ERROR("Unsupported audio format: %d (only PCM supported)", audio_format);
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    /* 1 (mono) or 2 (stereo) channels. */
    uint16_t num_channels = *(uint16_t *)(header + 22);
    if (num_channels != 1 && num_channels != 2) {
        AUDIO_ERROR("Unsupported number of channels: %d", num_channels);
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    /* Common sample rates. */
    uint32_t sample_rate = *(uint32_t *)(header + 24);
    if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 22050
        && sample_rate != 44100 && sample_rate != 48000) {
        AUDIO_ERROR("Uncommon sample rate: %lu Hz", sample_rate);
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    /* 8/16/24/32-bit samples. */
    uint16_t bits_per_sample = *(uint16_t *)(header + 34);
    if (bits_per_sample != 8 && bits_per_sample != 16
        && bits_per_sample != 24 && bits_per_sample != 32) {
        AUDIO_ERROR("Unsupported bits per sample: %d", bits_per_sample);
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    /* data subchunk marker. */
    if (memcmp(header + 36, "data", 4) != 0) {
        AUDIO_ERROR("Invalid data subchunk");
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    uint32_t file_size = *(uint32_t *)(header + 4) + 8;
    uint32_t data_size = *(uint32_t *)(header + 40);

    AUDIO_INFO("WAV File Info: %d channels, %lu Hz, %d bits, %lu bytes data, %lu bytes total",
              num_channels, sample_rate, bits_per_sample, data_size, file_size);

    fseek(file, original_position, SEEK_SET);
    return true;
}

/**
 * @brief Play a WAV file from the SD card through the speaker.
 *
 * Validates the header, then streams the PCM data in 512-sample
 * chunks: each sample is amplified 10x, clamped to int16 range,
 * and written to the I2S playback channel.
 *
 * @param filename Path to the WAV file (under the SD mount point).
 * @return ESP_OK on success.
 */
esp_err_t Audio_play_wav_sd(const char *filename)
{
    esp_err_t err = ESP_OK;

    if (filename == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *fh = fopen(filename, "rb");
    if (fh == NULL) {
        AUDIO_ERROR("Failed to open file");
        return ESP_ERR_INVALID_ARG;
    }

    if (!validate_wav_header(fh)) {
        AUDIO_ERROR("Invalid WAV file format: %s", filename);
        fclose(fh);
        return ESP_ERR_INVALID_ARG;
    }

    /* Skip the 44-byte header to reach PCM data. */
    if (fseek(fh, 44, SEEK_SET) != 0) {
        AUDIO_ERROR("Failed to seek file");
        fclose(fh);
        return ESP_FAIL;
    }

    /* 512 samples per chunk; output is stereo so twice the size. */
    const size_t SAMPLES_PER_BUFFER = 512;
    const size_t INPUT_BUFFER_SIZE = SAMPLES_PER_BUFFER * sizeof(int16_t);
    const size_t OUTPUT_BUFFER_SIZE = SAMPLES_PER_BUFFER * 2 * sizeof(int16_t);

    int16_t *input_buf = heap_caps_malloc(INPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    int16_t *output_buf = heap_caps_malloc(OUTPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (input_buf == NULL || output_buf == NULL) {
        AUDIO_ERROR("Failed to allocate audio buffers");
        if (input_buf) {
            free(input_buf);
        }
        if (output_buf) {
            free(output_buf);
        }
        fclose(fh);
        return ESP_ERR_NO_MEM;
    }

    size_t samples_read = 0;
    size_t bytes_to_write = 0;
    size_t bytes_written = 0;
    size_t total_samples = 0;
    int32_t volume_data = 0;

    /* Turn on the amplifier just before playback. */
    set_Audio_ctrl(true);

    while (1) {
        samples_read = fread(input_buf, sizeof(int16_t), SAMPLES_PER_BUFFER, fh);
        if (samples_read == 0) {
            break;
        }

        /* Amplify and clamp each sample. */
        for (size_t i = 0; i < samples_read; i++) {
            volume_data = input_buf[i] * 10;
            if (volume_data > 32767) {
                volume_data = 32767;
            } else if (volume_data < -32768) {
                volume_data = -32768;
            }
            output_buf[i] = (int16_t)volume_data;
        }

        bytes_to_write = samples_read * sizeof(int16_t);
        bytes_written = 0;
        err = i2s_channel_write(tx_chan, output_buf, bytes_to_write,
                                &bytes_written, portMAX_DELAY);
        if (err != ESP_OK || bytes_written != bytes_to_write) {
            AUDIO_ERROR("I2S write failed: %s, written: %d/%d",
                       esp_err_to_name(err), bytes_written, bytes_to_write);
            break;
        }
        total_samples += samples_read;
    }

    /* Mute and free resources. */
    set_Audio_ctrl(false);
    free(input_buf);
    free(output_buf);
    fclose(fh);

    AUDIO_INFO("Audio playback completed: %d samples", total_samples);
    return err;
}
