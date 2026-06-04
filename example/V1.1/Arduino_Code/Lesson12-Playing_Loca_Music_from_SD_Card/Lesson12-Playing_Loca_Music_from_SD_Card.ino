/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs

#include <string.h>         // Include standard string manipulation functions
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes
#include <esp_ldo_regulator.h>  // ESP32-P4 specific LDO management

#include <ESP_I2S.h>        // ESP32 I2S Library

#include <sys/unistd.h>         // Include system calls for file handling
#include <sys/stat.h>           // Include functions for file status and permissions
#include <esp_vfs_fat.h>        // Include ESP-IDF FAT filesystem support for SD card
#include <sdmmc_cmd.h>          // Include SDMMC card command definitions and helpers
#include <driver/sdmmc_host.h>  // Include SDMMC host driver for SD card communication
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ORIGINAL(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__);
#define PRINTF_PRINT(fmt, ...)    Serial.print(fmt);
#define PRINTF_LN(fmt, ...)       Serial.println(fmt);

#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("[ERROR] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_WARN(fmt, ...)       do { \
                                        Serial.print("[WARN] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_INFO(fmt, ...)       do { \
                                        Serial.print("[INFO] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_DEBUG(fmt, ...)      do { \
                                        Serial.print("[DEBUG] "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)

#define AUDIO_INFO(fmt, ...)        PRINTF_INFO(fmt, ##__VA_ARGS__)   // Info level log macro
#define AUDIO_ERROR(fmt, ...)       PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Error level log macro

#define SD_INFO(fmt, ...)           PRINTF_INFO(fmt, ##__VA_ARGS__) 
#define SD_ERROR(fmt, ...)          PRINTF_ERROR(fmt, ##__VA_ARGS__)



#define EXAMPLE_MAX_CHAR_SIZE 64   // Maximum character buffer size for file read/write operations
#define SD_MOUNT_POINT "/sdcard"   // Default SD card mount point path
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
static sdmmc_card_t *card;
const char sd_mount_point[] = SD_MOUNT_POINT;

/*microphone*/
static I2SClass i2s_mic; // Create an I2SClass microphone instance
/*loudspeaker*/
static I2SClass i2s_spk; // Create an I2SClass speaker instance

static TaskHandle_t mic_task_handle = NULL;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

esp_err_t sd_init()
{
    esp_err_t err = ESP_OK;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        // if SD card file system format is not fat32, mount failed unless ".format_if_mount_failed = true".
        .format_if_mount_failed = false,    
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host =  SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = 10000;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = (gpio_num_t)SD_GPIO_MMC_CLK;
    slot_config.cmd = (gpio_num_t)SD_GPIO_MMC_CMD;
    slot_config.d0 = (gpio_num_t)SD_GPIO_MMC_D0;
    slot_config.width = 1; // Single-line SDIO
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    SD_INFO("Mounting filesystem");
    err = esp_vfs_fat_sdmmc_mount(sd_mount_point, &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            SD_INFO("Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            SD_INFO("Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(err));
        }
        return err;
    }
    SD_INFO("Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
    return err;
}

esp_err_t format_sd_card()
{
    esp_err_t err = ESP_OK;
    err = esp_vfs_fat_sdcard_format(sd_mount_point, card);
    if (err != ESP_OK)
    {
        SD_ERROR("Failed to format FATFS (%s)", esp_err_to_name(err));
        return err;
    }
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

    AUDIO_INFO("WAV File Info: %d channels, %lu Hz, %d bits, %lu bytes data, %lu bytes total", 
                            num_channels, sample_rate, bits_per_sample, data_size, file_size);
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
    int16_t *input_buf = (int16_t*)heap_caps_malloc(INPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
    int16_t *output_buf = (int16_t*)heap_caps_malloc(OUTPUT_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_32BIT);

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
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_ENABLE);   // Enable audio power
    while (1)
    {
        samples_read = fread(input_buf, sizeof(int16_t), SAMPLES_PER_BUFFER, fh);
        if (samples_read == 0)
            break;
        for (size_t i = 0; i < samples_read; i++) /*convert mono to stereo*/
        {
            volume_data = input_buf[i] * 1; /*Linear multiplication*/
            if (volume_data > 32767)
                volume_data = 32767;
            else if (volume_data < -32768)
                volume_data = -32768;
            output_buf[i] = (int16_t)volume_data;     /*Left channel*/
        }
        
        bytes_to_write = samples_read * sizeof(int16_t);
        bytes_written = 0;
        
        bytes_written = i2s_spk.write((uint8_t*)output_buf, bytes_to_write);  /*I2S write data*/
        if (bytes_written != bytes_to_write)
        {
            AUDIO_ERROR("I2S write failed: %s, written: %d/%d", esp_err_to_name(err), bytes_written, bytes_to_write);
            break;
        }
        total_samples += samples_read;
    }
    /*Cleanup*/
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_DISABLE);   // Disable audio power
    free(input_buf);
    free(output_buf);
    fclose(fh);
    AUDIO_INFO("Audio playback completed: %d samples", total_samples);
    return err;
}

void sd_card_init(void)   // System initialization function
{
    esp_err_t err = ESP_OK;   // Variable to store error codes
    SD_INFO("----------SD card test program start----------\r\n");   // Print program start message
    // Initialize SD card
    err = sd_init();   // Call SD card initialization function
    // Check if initialization failed
    while (ESP_OK != err) { 
        SD_ERROR("%s initialization failed [ %s ]", "SD card", esp_err_to_name(err));   // Print module name and error description
        delay (1000);
        err = sd_init();   // Call SD card initialization function
    }
}

void mic_loudspeaker_init()
{
    pinMode(AUDIO_GPIO_CTRL, OUTPUT);
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_DISABLE);   // Disable audio power
    
    i2s_mic.setPinsPdmRx(MIC_GPIO_CLK, MIC_GPIO_SDIN); // Configure pins for microphone input
    // Start I2S at 16 kHz frequency, 16-bit depth, mono
    if (!i2s_mic.begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
        Serial.println("PDM input initialization failed!");
        while (1) delay(1000);  // Halt execution
    }
    
    i2s_spk.setPins(AUDIO_GPIO_BCLK, AUDIO_GPIO_LRCLK, AUDIO_GPIO_SDATA);  // BCLK, LRCLK, DOUT
    // Start I2S with the same parameters, but in output mode, using mono
    if (!i2s_spk.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
        Serial.println("I2S output mode initialization failed!");
        while (1) delay(1000);
    }
}


void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);
    
    // Initialize sd card
    sd_card_init();   // Call initialization function to set up SD card and other components

    mic_loudspeaker_init();

    Audio_play_wav_sd(SD_MOUNT_POINT"/huahai.wav");
}

void loop() {
    // put your main code here, to run repeatedly:
}