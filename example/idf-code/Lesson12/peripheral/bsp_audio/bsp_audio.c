/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_audio.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

i2s_chan_handle_t tx_chan;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

esp_err_t audio_init()
{
    esp_err_t err = ESP_OK;
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_1,         /*Use I2S controller 1*/
        .role = I2S_ROLE_MASTER, /*I2S acts as master (generates clock signals)*/
        .dma_desc_num = 6,       /*Number of DMA descriptors for buffer management*/
        .dma_frame_num = 256,    /*Number of frames per DMA descriptor*/
        .auto_clear = true,      /*Automatically clear DMA buffer on underflow*/
        .intr_priority = 0,      /*Interrupt priority level*/
    }; /*I2S channel configuration for transmitter (audio output)*/
    err = i2s_new_channel(&chan_cfg, &tx_chan, NULL); /*Create new I2S channel (transmit channel only, no receive channel)*/
    if (err != ESP_OK)
        return err;
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 16000,                /*Audio sample rate: 16kHz*/
            .clk_src = I2S_CLK_SRC_DEFAULT,         /*Default clock source*/
            .mclk_multiple = I2S_MCLK_MULTIPLE_256, /*Master clock multiplier*/
        },                                          /* Clock configuration*/
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT, /*16-bit audio samples*/
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,  /*Auto-calculate slot width*/
            .slot_mode = I2S_SLOT_MODE_STEREO,          /*Stereo audio (2 channels)*/
            .slot_mask = I2S_STD_SLOT_BOTH,             /*Enable both left and right channels*/
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,       /*Word select signal width*/
            .ws_pol = false,                            /*Word select polarity (normal)*/
            .bit_shift = true,                          /*Enable bit shift in data frame*/
            .left_align = true,                         /*Left-aligned data in slot*/
            .big_endian = false,                        /*Little-endian byte order*/
            .bit_order_lsb = false,                     /*MSB first bit order*/
        },                                              /* Audio slot/data format configuration*/
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,  /*Master clock not used*/
            .bclk = AUDIO_GPIO_BCLK,  /*Bit clock pin*/
            .ws = AUDIO_GPIO_LRCLK,   /*Word select (left/right clock) pin*/
            .dout = AUDIO_GPIO_SDATA, /*Serial data output pin*/
            .din = I2S_GPIO_UNUSED,   /*Data input not used (output only)*/
            .invert_flags = {
                .mclk_inv = false, /*Don't invert master clock*/
                .bclk_inv = false, /*Don't invert bit clock*/
                .ws_inv = false,   /*Don't invert word select*/
            },                     /*Signal inversion flags*/
        },                         /*GPIO pin configuration for I2S signals*/
    }; /*Standard I2S configuration for audio playback*/
    err = i2s_channel_init_std_mode(tx_chan, &std_cfg); /*Initialize I2S channel in standard mode for audio output*/
    if (err != ESP_OK)
        return err;

    err = i2s_channel_enable(tx_chan); /*Enable the I2S channel to start audio transmission*/
    if (err != ESP_OK)
        return err;

    return err;
}

esp_err_t audio_ctrl_init()
{
    esp_err_t err = ESP_OK;
    const gpio_config_t gpio_cofig = {
        .pin_bit_mask = 1ULL << AUDIO_GPIO_CTRL, /* GPIO pin: set with bit mask, each bit maps to a GPIO */
        .mode = GPIO_MODE_OUTPUT,                /* GPIO mode: set input/output mode                     */
        .pull_up_en = false,                     /* GPIO pull-up                                         */
        .pull_down_en = false,                   /* GPIO pull-down                                       */
        .intr_type = GPIO_INTR_DISABLE,          /* GPIO interrupt type                                  */
    };
    err = gpio_config(&gpio_cofig); /*Configure GPIO*/
    if (err != ESP_OK)
        return err;
    return err;
}

esp_err_t set_Audio_ctrl(bool state)
{
    esp_err_t err = ESP_OK;
    bool status = !state;                          /*Invert the state (the sound is on when the voltage is low, and off when the voltage is high)*/
    err = gpio_set_level(AUDIO_GPIO_CTRL, status); /*Set the Corresponding Output Level of GPIO*/
    return err;
}

bool validate_wav_header(FILE *file)
{
    if (file == NULL)
    {
        AUDIO_ERROR("File pointer is NULL");
        return false;
    }
    long original_position = ftell(file); /*Store current file position to restore later*/
    if (original_position == -1)
    {
        AUDIO_ERROR("Cannot get current file position");
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) /*Rewind to beginning of file*/
    {
        AUDIO_ERROR("Cannot seek to file beginning");
        return false;
    }
    uint8_t header[44]; /*Read and validate WAV header*/
    size_t bytes_read = fread(header, 1, 44, file);
    if (bytes_read != 44)
    {
        AUDIO_ERROR("Cannot read complete WAV header (%d bytes)", bytes_read);
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    if (memcmp(header, "RIFF", 4) != 0) /*Validate RIFF chunk descriptor*/
    {
        AUDIO_ERROR("Invalid RIFF header");
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    if (memcmp(header + 8, "WAVE", 4) != 0) /*Validate WAVE format*/
    {
        AUDIO_ERROR("Invalid WAVE format");
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    if (memcmp(header + 12, "fmt ", 4) != 0) /*Validate fmt subchunk*/
    {
        AUDIO_ERROR("Invalid fmt subchunk");
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    uint16_t audio_format = *(uint16_t *)(header + 20); /*Check audio format (should be 1 for PCM)*/
    if (audio_format != 1)
    {
        AUDIO_ERROR("Unsupported audio format: %d (only PCM supported)", audio_format);
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    uint16_t num_channels = *(uint16_t *)(header + 22); /*Check number of channels (support mono and stereo)*/
    if (num_channels != 1 && num_channels != 2)
    {
        AUDIO_ERROR("Unsupported number of channels: %d", num_channels);
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    uint32_t sample_rate = *(uint32_t *)(header + 24); /*Check sample rate (support common rates)*/
    if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 22050 && sample_rate != 44100 && sample_rate != 48000)
    {
        AUDIO_ERROR("Uncommon sample rate: %lu Hz", sample_rate);
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    uint16_t bits_per_sample = *(uint16_t *)(header + 34); /*Check bits per sample (support 8, 16, 24, 32)*/
    if (bits_per_sample != 8 && bits_per_sample != 16 && bits_per_sample != 24 && bits_per_sample != 32)
    {
        AUDIO_ERROR("Unsupported bits per sample: %d", bits_per_sample);
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    if (memcmp(header + 36, "data", 4) != 0) /*Validate data subchunk*/
    {
        AUDIO_ERROR("Invalid data subchunk");
        fseek(file, original_position, SEEK_SET);
        return false;
    }
    /*Get file size from RIFF header for additional validation*/
    uint32_t file_size = *(uint32_t *)(header + 4) + 8; // RIFF block size + 8-byte header
    uint32_t data_size = *(uint32_t *)(header + 40);

    AUDIO_INFO("WAV File Info: %d channels, %lu Hz, %d bits, %lu bytes data, %lu bytes total", num_channels, sample_rate, bits_per_sample, data_size, file_size);
    /*Restore original file position*/
    fseek(file, original_position, SEEK_SET);
    return true;
}

esp_err_t Audio_play_wav_sd(const char *filename)
{
    esp_err_t err = ESP_OK;
    if (filename == NULL)
        return ESP_ERR_INVALID_ARG;
    FILE *fh = fopen(filename, "rb");
    if (fh == NULL)
    {
        AUDIO_ERROR("Failed to open file");
        return ESP_ERR_INVALID_ARG;
    }
    if (!validate_wav_header(fh)) /*Validate WAV header*/
    {
        AUDIO_ERROR("Invalid WAV file format: %s", filename);
        fclose(fh);
        return ESP_ERR_INVALID_ARG;
    }
    if (fseek(fh, 44, SEEK_SET) != 0) /*Skip 44-byte WAV header*/
    {
        AUDIO_ERROR("Failed to seek file");
        fclose(fh);
        return ESP_FAIL;
    }
    /*Buffer configuration*/
    const size_t SAMPLES_PER_BUFFER = 512;
    const size_t INPUT_BUFFER_SIZE = SAMPLES_PER_BUFFER * sizeof(int16_t);
    const size_t OUTPUT_BUFFER_SIZE = SAMPLES_PER_BUFFER * 2 * sizeof(int16_t);
    /* Allocate buffers*/
    int16_t *input_buf = heap_caps_malloc(INPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    int16_t *output_buf = heap_caps_malloc(OUTPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (input_buf == NULL || output_buf == NULL)
    {
        AUDIO_ERROR("Failed to allocate audio buffers");
        if (input_buf)
            free(input_buf);
        if (output_buf)
            free(output_buf);
        fclose(fh);
        return ESP_ERR_NO_MEM;
    }
    size_t samples_read = 0;
    size_t bytes_to_write = 0;
    size_t bytes_written = 0;
    size_t total_samples = 0;
    int32_t volume_data = 0;
    set_Audio_ctrl(true); /*Enable audio hardware*/
    while (1)
    {
        samples_read = fread(input_buf, sizeof(int16_t), SAMPLES_PER_BUFFER, fh);
        if (samples_read == 0)
            break;
        for (size_t i = 0; i < samples_read; i++) /*convert mono to stereo*/
        {
            volume_data = input_buf[i] * 10; /*Linear multiplication*/
            if (volume_data > 32767)
                volume_data = 32767;
            else if (volume_data < -32768)
                volume_data = -32768;
            output_buf[i] = (int16_t)volume_data;     /*Left channel*/
        }
        bytes_to_write = samples_read * sizeof(int16_t);
        bytes_written = 0;
        err = i2s_channel_write(tx_chan, output_buf, bytes_to_write, &bytes_written, portMAX_DELAY); /*I2S write data*/
        if (err != ESP_OK || bytes_written != bytes_to_write)
        {
            AUDIO_ERROR("I2S write failed: %s, written: %d/%d", esp_err_to_name(err), bytes_written, bytes_to_write);
            break;
        }
        total_samples += samples_read;
    }
    /*Cleanup*/
    set_Audio_ctrl(false);
    free(input_buf);
    free(output_buf);
    fclose(fh);
    AUDIO_INFO("Audio playback completed: %d samples", total_samples);
    return err;
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/