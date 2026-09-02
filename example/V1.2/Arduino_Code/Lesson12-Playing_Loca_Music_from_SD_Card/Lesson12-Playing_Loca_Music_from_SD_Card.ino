/**
 * @file Lesson12-Playing_Loca_Music_from_SD_Card.ino
 * @brief Teaching example that demonstrates how to read a WAV file from the SD card and play it through I2S.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

/*---------------------------------------------------------------
 * Dependencies
 * Libraries and board interfaces used by this lesson.
 *--------------------------------------------------------------*/
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs
#include <esp_arduino_version.h>

#include <string.h>         // Include standard string manipulation functions
#include <limits.h>
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes
#include <esp_ldo_regulator.h>  // ESP32-P4 specific LDO management

#include <ESP_I2S.h>        // ESP32 I2S Library

#include <sys/unistd.h>         // Include system calls for file handling
#include <sys/stat.h>           // Include functions for file status and permissions
#include <esp_vfs_fat.h>        // Include ESP-IDF FAT filesystem support for SD card
#include <sdmmc_cmd.h>          // Include SDMMC card command definitions and helpers
#include <driver/sdmmc_host.h>  // Include SDMMC host driver for SD card communication

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 8)
#error "This lesson requires ESP32 Arduino Core 3.3.8 or newer"
#endif
/*---------------------------------------------------------------
 * Configuration constants
 * Compile-time settings and reusable logging helpers.
 *--------------------------------------------------------------*/
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
/*---------------------------------------------------------------
 * Shared lesson state
 * State shared by callbacks, tasks, and Arduino entry points.
 *--------------------------------------------------------------*/
// Holds the card descriptor returned by the SDMMC mount routine.
static sdmmc_card_t *card;
// Stores the mount path reused by all file operations.
const char sd_mount_point[] = SD_MOUNT_POINT;

/*microphone*/
static I2SClass i2s_mic; // Create an I2SClass microphone instance
/*loudspeaker*/
static I2SClass i2s_spk; // Create an I2SClass speaker instance

struct WavInfo {
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t block_align;
    uint32_t data_size;
    long data_offset;
};


/*---------------------------------------------------------------
 * Lesson helper functions
 * Keep related operations together so the execution flow is easy to follow.
 *--------------------------------------------------------------*/
/**
 * @brief Initialize the SDMMC host and mount the card file system.
 *
 * Parameters: None.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called during lesson setup before the related hardware is used.
 */
esp_err_t sd_init()
{
    esp_err_t err = ESP_OK;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        // if SD card file system format is not fat32, mount failed unless ".format_if_mount_failed = true".
        .format_if_mount_failed = false,    
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false,
        .use_one_fat = false,
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

/**
 * @brief Format the mounted SD card as FAT after explicit invocation.
 *
 * Parameters: None.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called by the lesson workflow when this helper operation is required.
 */
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

/**
 * @brief Validate the RIFF/WAVE metadata before attempting audio playback.
 *
 * @param file Open WAV file positioned at its header.
 * @return true when the file contains supported PCM WAV metadata; otherwise false.
 * @note Called by the lesson workflow when this helper operation is required.
 */
static uint16_t read_le16(const uint8_t *data)
{
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
}

static uint32_t read_le32(const uint8_t *data)
{
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

bool read_wav_info(FILE *file, WavInfo *info)
{
    if (file == nullptr || info == nullptr)
    {
        AUDIO_ERROR("Invalid WAV parser argument");
        return false;
    }
    *info = {};

    const long original_position = ftell(file);
    if (original_position == -1)
    {
        AUDIO_ERROR("Cannot get current file position");
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        AUDIO_ERROR("Cannot seek to file beginning");
        return false;
    }

    uint8_t riff_header[12];
    if (fread(riff_header, 1, sizeof(riff_header), file) != sizeof(riff_header) ||
        memcmp(riff_header, "RIFF", 4) != 0 || memcmp(riff_header + 8, "WAVE", 4) != 0)
    {
        AUDIO_ERROR("Invalid RIFF/WAVE header");
        fseek(file, original_position, SEEK_SET);
        return false;
    }

    bool fmt_found = false;
    bool data_found = false;
    uint16_t audio_format = 0;
    uint8_t chunk_header[8];

    while (fread(chunk_header, 1, sizeof(chunk_header), file) == sizeof(chunk_header))
    {
        const uint32_t chunk_size = read_le32(chunk_header + 4);
        const long chunk_data_offset = ftell(file);
        if (chunk_data_offset < 0) {
            break;
        }

        if (memcmp(chunk_header, "fmt ", 4) == 0)
        {
            if (chunk_size < 16) {
                break;
            }
            uint8_t fmt_data[16];
            if (fread(fmt_data, 1, sizeof(fmt_data), file) != sizeof(fmt_data)) {
                break;
            }
            audio_format = read_le16(fmt_data);
            info->channels = read_le16(fmt_data + 2);
            info->sample_rate = read_le32(fmt_data + 4);
            info->block_align = read_le16(fmt_data + 12);
            info->bits_per_sample = read_le16(fmt_data + 14);
            fmt_found = true;
        }
        else if (memcmp(chunk_header, "data", 4) == 0)
        {
            info->data_offset = chunk_data_offset;
            info->data_size = chunk_size;
            data_found = true;
            break;
        }

        const uint32_t padded_size = chunk_size + (chunk_size & 1U);
        if (padded_size < chunk_size || padded_size > static_cast<uint32_t>(LONG_MAX) ||
            chunk_data_offset > LONG_MAX - static_cast<long>(padded_size) ||
            fseek(file, chunk_data_offset + static_cast<long>(padded_size), SEEK_SET) != 0)
        {
            break;
        }
    }

    const uint16_t expected_block_align = static_cast<uint16_t>(
        info->channels * (info->bits_per_sample / 8)
    );
    const bool supported_bits = info->bits_per_sample == 8 || info->bits_per_sample == 16 ||
                                info->bits_per_sample == 24 || info->bits_per_sample == 32;
    const bool valid = fmt_found && data_found && audio_format == 1 &&
                       (info->channels == 1 || info->channels == 2) &&
                       info->sample_rate > 0 && supported_bits &&
                       info->block_align == expected_block_align && expected_block_align > 0;

    fseek(file, original_position, SEEK_SET);
    if (!valid)
    {
        AUDIO_ERROR("Unsupported or incomplete PCM WAV metadata");
        return false;
    }

    AUDIO_INFO(
        "WAV File Info: %u channels, %lu Hz, %u bits, %lu bytes data",
        info->channels, static_cast<unsigned long>(info->sample_rate),
        info->bits_per_sample, static_cast<unsigned long>(info->data_size)
    );
    return true;
}

/**
 * @brief Stream PCM samples from a WAV file on the SD card to the I2S speaker.
 *
 * @param filename Path of the file to access on the mounted SD card.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson requests the corresponding data operation.
 */
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
    WavInfo wav_info = {};
    if (!read_wav_info(fh, &wav_info))
    {
        AUDIO_ERROR("Invalid WAV file format: %s", filename);
        fclose(fh);
        return ESP_ERR_INVALID_ARG;
    }
    if (!i2s_spk.configureTX(
            wav_info.sample_rate,
            static_cast<i2s_data_bit_width_t>(wav_info.bits_per_sample),
            wav_info.channels == 1 ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO,
            I2S_STD_SLOT_BOTH))
    {
        AUDIO_ERROR("Failed to configure I2S for WAV metadata");
        fclose(fh);
        return ESP_FAIL;
    }
    if (fseek(fh, wav_info.data_offset, SEEK_SET) != 0)
    {
        AUDIO_ERROR("Failed to seek file");
        fclose(fh);
        return ESP_FAIL;
    }
    /*Buffer configuration*/
    static constexpr size_t AUDIO_BUFFER_SIZE = 4096;
    uint8_t *audio_buf = static_cast<uint8_t *>(
        heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    );
    if (audio_buf == nullptr) {
        audio_buf = static_cast<uint8_t *>(heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_8BIT));
    }
    if (audio_buf == nullptr)
    {
        AUDIO_ERROR("Failed to allocate audio buffers");
        fclose(fh);
        return ESP_ERR_NO_MEM;
    }

    size_t remaining = wav_info.data_size;
    size_t total_bytes = 0;
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_ENABLE);   // Enable audio power
    while (remaining > 0)
    {
        size_t bytes_to_read = min(remaining, AUDIO_BUFFER_SIZE);
        bytes_to_read -= bytes_to_read % wav_info.block_align;
        if (bytes_to_read == 0)
        {
            AUDIO_ERROR("WAV data is not aligned to complete audio frames");
            err = ESP_ERR_INVALID_SIZE;
            break;
        }

        const size_t bytes_read = fread(audio_buf, 1, bytes_to_read, fh);
        if (bytes_read != bytes_to_read)
        {
            AUDIO_ERROR("WAV data is truncated: read %zu/%zu bytes", bytes_read, bytes_to_read);
            err = ESP_FAIL;
            break;
        }

        const size_t bytes_written = i2s_spk.write(audio_buf, bytes_read);
        if (bytes_written != bytes_read)
        {
            AUDIO_ERROR(
                "I2S write failed: error=%d, written=%zu/%zu",
                i2s_spk.lastError(), bytes_written, bytes_read
            );
            err = ESP_FAIL;
            break;
        }
        remaining -= bytes_read;
        total_bytes += bytes_read;
    }
    /*Cleanup*/
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_DISABLE);   // Disable audio power
    free(audio_buf);
    fclose(fh);
    AUDIO_INFO("Audio playback completed: %zu bytes", total_bytes);
    return err;
}

/**
 * @brief Mount the SD card and report its file-system information.
 *
 * Parameters: None.
 * @return None.
 * @note Called during lesson setup before the related hardware is used.
 */
void sd_card_init(void)   
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

/**
 * @brief Configure the I2S audio channels used by the microphone and speaker.
 *
 * Parameters: None.
 * @return None.
 * @note Called during lesson setup before the related hardware is used.
 */
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


/*---------------------------------------------------------------
 * Arduino entry points
 * The Arduino runtime calls setup() once and loop() repeatedly.
 *--------------------------------------------------------------*/
/**
 * @brief Initialize the hardware and services needed to read a WAV file from the SD card and play it through I2S.
 *
 * Parameters: None.
 * @return None.
 * @note Called once by the Arduino runtime after reset.
 */
void setup() {

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);
    
    // Initialize sd card
    sd_card_init();   // Call initialization function to set up SD card and other components

    mic_loudspeaker_init();

    const esp_err_t playback_result = Audio_play_wav_sd(SD_MOUNT_POINT"/huahai.wav");
    if (playback_result != ESP_OK) {
        AUDIO_ERROR("Audio playback failed: %s", esp_err_to_name(playback_result));
    }
}

/**
 * @brief Continue the runtime workflow used to read a WAV file from the SD card and play it through I2S.
 *
 * Parameters: None.
 * @return None.
 * @note Called repeatedly by the Arduino runtime after setup() returns.
 */
void loop() {
}
