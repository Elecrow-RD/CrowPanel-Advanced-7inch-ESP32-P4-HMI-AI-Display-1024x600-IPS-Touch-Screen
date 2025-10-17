/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_mic.h"    // Include the microphone module header file
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
i2s_chan_handle_t rx_chan;    // Global I2S receive channel handle for microphone
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

esp_err_t mic_init()    // Initialize the microphone I2S channel
{
    esp_err_t err = ESP_OK;    // Initialize error variable

    i2s_chan_config_t rx_chan_cfg = {    // Configure I2S channel parameters
        .id = I2S_NUM_0,                 // Use I2S port 0
        .role = I2S_ROLE_MASTER,         // Set as master
        .dma_desc_num = 6,               // Number of DMA descriptors
        .dma_frame_num = 256,            // Number of frames per DMA descriptor
        .auto_clear_after_cb = true,     // Auto-clear DMA after callback
        .auto_clear_before_cb = true,    // Auto-clear DMA before callback
        .allow_pd = false,               // Disallow power-down mode
        .intr_priority = 0,              // Interrupt priority
    };
    err = i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan);    // Create I2S receive channel
    if (err != ESP_OK)
        return err;    // Return if failed

    i2s_pdm_rx_config_t pdm_rx_cfg = {    // Configure PDM receive parameters
        .clk_cfg = {                       // Clock configuration
            .sample_rate_hz = MIC_SAMPLE_RATE,    // Set sample rate 16kHz
            .clk_src = I2S_CLK_SRC_DEFAULT,       // Use default clock source
            .mclk_multiple = I2S_MCLK_MULTIPLE_256, // MCLK multiplier
            .dn_sample_mode = I2S_PDM_DSR_8S,       // Downsample mode
            .bclk_div = 8,                           // Bit clock divider
        },
        /* The data bit-width of PDM mode is fixed to 16 */
        .slot_cfg = {                   // Slot configuration
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,    // 16-bit data width
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,     // Slot bit width auto
            .slot_mode = I2S_SLOT_MODE_MONO,              // Mono mode
            .slot_mask = I2S_PDM_SLOT_LEFT,              // Left channel used
            .hp_en = true,                               // Enable high-pass filter
            .hp_cut_off_freq_hz = 35.5,                  // High-pass cutoff frequency
            .amplify_num = 1,                            // Amplification factor
        },
        .gpio_cfg = {                   // GPIO configuration
            .clk = MIC_GPIO_CLK,        // Clock pin
            .din = MIC_GPIO_SDIN2,      // Data input pin
            .invert_flags = {
                .clk_inv = false,      // Clock polarity not inverted
            },
        },
    };
    err = i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg);    // Initialize PDM RX mode
    if (err != ESP_OK)
        return err;    // Return if failed
    err = i2s_channel_enable(rx_chan);    // Enable the RX channel
    if (err != ESP_OK)
        return err;    // Return if failed
    return err;    // Return success
}

esp_err_t mic_read_to_audio(size_t rec_seconds)    // Record audio from microphone and play
{
    esp_err_t err = ESP_OK;     // Initialize error variable
    size_t bytes_read = 0;      // Bytes read from microphone
    size_t bytes_write = 0;     // Bytes written to audio output

    if (rec_seconds > 60)    // Check maximum recording duration
    {
        MIC_INFO("Exceeding the maximum recording duration");    // Log warning
        return ESP_FAIL;    // Return failure
    }

    size_t rec_size = rec_seconds * BYTE_RATE;    // Calculate total bytes to record
    i2s_chan_handle_t write_handle = get_audio_handle();    // Get audio output channel handle

    int16_t *read_buf = heap_caps_malloc(rec_size, MALLOC_CAP_SPIRAM);    // Allocate buffer for recording
    if (NULL == read_buf) {
        MIC_INFO("mic read_buf fail to apply");    // Log allocation failure
        return ESP_FAIL;    // Return failure
    }
    memset(read_buf, 0, rec_size);    // Clear read buffer

    int16_t *write_buf = heap_caps_malloc(rec_size * 2, MALLOC_CAP_SPIRAM);    // Allocate buffer for playback (stereo)
    if (NULL == write_buf) {
        MIC_INFO("mic write_buf fail to apply");    // Log allocation failure
        return ESP_FAIL;    // Return failure
    }
    memset(write_buf, 0, rec_size * 2);    // Clear write buffer

    MIC_INFO("Start Recording %d of audio data", rec_seconds);    // Log start recording
    err = i2s_channel_read(rx_chan, read_buf, rec_size, &bytes_read, portMAX_DELAY);    // Read microphone data
    if (err != ESP_OK)
    {
        MIC_INFO("read mic data fail");    // Log read failure
        return err;    // Return error
    }
    if (bytes_read != rec_size)    // Check if read size is correct
    {
        MIC_INFO("read mic data num error");    // Log size mismatch
        return err;    // Return error
    }

    int32_t data;                     // Temporary variable for processing
    int32_t array_size = rec_size / 2;    // Number of samples (16-bit each)
    for (size_t i = 0, j = 0; i < array_size; i++, j += 2)
    {
        data = read_buf[i] * 10;             // Amplify sample value
        if (data > 32767)                    // Limit to int16 max
            data = 32767;
        else if (data < -32768)              // Limit to int16 min
            data = -32768;
        write_buf[j] = data;                 // Left channel
        write_buf[j + 1] = data;             // Right channel (duplicate for stereo)
    }

    MIC_INFO("Start play audio data");    // Log playback start
    set_Audio_ctrl(true);                  // Turn on audio amplifier
    err = i2s_channel_write(write_handle, write_buf, rec_size * 2, &bytes_write, portMAX_DELAY);    // Write data to audio output
    if (err != ESP_OK)
    {
        set_Audio_ctrl(false);             // Turn off amplifier if write fails
        heap_caps_free(read_buf);          // Free read buffer
        heap_caps_free(write_buf);         // Free write buffer
        return err;                        // Return error
    }
    set_Audio_ctrl(false);                 // Turn off amplifier after playback
    heap_caps_free(read_buf);              // Free read buffer
    heap_caps_free(write_buf);             // Free write buffer
    return err;                            // Return success
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
