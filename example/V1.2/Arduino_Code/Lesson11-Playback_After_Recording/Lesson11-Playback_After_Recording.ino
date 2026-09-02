/**
 * @file Lesson11-Playback_After_Recording.ino
 * @brief Teaching example that demonstrates how to record microphone audio and play the captured samples.
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
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes

#include <ESP_I2S.h>        // ESP32 I2S Library

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

#define MAIN_INFO(fmt, ...)         PRINTF_INFO(fmt, ##__VA_ARGS__)   // Info level log macro
#define MAIN_ERROR(fmt, ...)        PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Error level log macro
/*---------------------------------------------------------------
 * Shared lesson state
 * State shared by callbacks, tasks, and Arduino entry points.
 *--------------------------------------------------------------*/


/*microphone*/
static I2SClass i2s_mic; // Create an I2SClass microphone instance
/*loudspeaker*/
static I2SClass i2s_spk; // Create an I2SClass speaker instance

static constexpr size_t RECORD_SECONDS = 5;
static constexpr size_t WAV_HEADER_BYTES = 44;
static constexpr size_t STARTUP_SKIP_BYTES = 4000;

/*---------------------------------------------------------------
 * Lesson helper functions
 * Keep related operations together so the execution flow is easy to follow.
 *--------------------------------------------------------------*/
/**
 * @brief Capture microphone samples and play the recorded buffer through the speaker.
 *
 * Parameters: None.
 * @return None.
 * @note Called when the lesson requests the corresponding data operation.
 */
void recording()
{
    size_t wav_size = 0;
    Serial.printf("Recording %zu seconds of audio data...\n", RECORD_SECONDS);
    uint8_t *origin_buffer = i2s_mic.recordWAV(RECORD_SECONDS, &wav_size);
    if (origin_buffer == nullptr || wav_size <= WAV_HEADER_BYTES) {
        Serial.printf("Recording failed, I2S error: %d\n", i2s_mic.lastError());
        free(origin_buffer);
        return;
    }

    const size_t pcm_bytes = wav_size - WAV_HEADER_BYTES;
    if ((pcm_bytes % sizeof(int16_t)) != 0) {
        Serial.println("Recorded PCM data is not 16-bit sample aligned.");
        free(origin_buffer);
        return;
    }

    int16_t *read_buffer = reinterpret_cast<int16_t *>(origin_buffer + WAV_HEADER_BYTES);
    int16_t *out_buffer = static_cast<int16_t *>(
        heap_caps_malloc(pcm_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    );
    if (out_buffer == nullptr) {
        Serial.println("Memory allocation failure!");
        free(origin_buffer);
        return;
    }

    const size_t num_samples = pcm_bytes / sizeof(int16_t);
    Serial.printf("read_bytes = %zu\n", pcm_bytes);
    Serial.println("Recording end");

    // Find the maximum absolute value (peak) in the raw data
    int32_t max_val = 0;
    for (size_t i = 0; i < num_samples; i++) {
        // Use abs() function to ensure absolute value is taken
        int32_t current_val = abs(read_buffer[i]); 
        if (current_val > max_val) {
            max_val = current_val;
        }
    }
    
    /* Simply calculate the volume gain to prevent popping sounds */
    float safe_gain = 1.0f;
    if (max_val > 0) {
        safe_gain = 32767.0f / max_val;
    }
    
    float desired_amplification = 20.0f;  // Maximum amplification factor
    float final_gain = desired_amplification;
    if (final_gain > safe_gain) {
        final_gain = safe_gain * 1.5f;
        Serial.printf("Warning: Clipping prevented. Max safe gain used: %.2f\n", final_gain);
    } else {
        Serial.printf("Applying desired gain: %.2f\n", final_gain);
    }

    // Multiply the original data by the gain coefficient
    for (size_t i=0; i<num_samples; i+=1) {
        if (read_buffer[i] < 0) {
            if (-32768 / final_gain < read_buffer[i]) {
                out_buffer[i] = (read_buffer[i]) * final_gain;
            } else {
                out_buffer[i] = -32768;
            }
        } else {
            if (read_buffer[i]< 32767 / final_gain) {
                out_buffer[i] = (read_buffer[i]) * final_gain;
            } else {
                out_buffer[i] = 32767;
            }
        }
    }
    Serial.println("Playing the recorded audio...");
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_ENABLE);  // Enable audio power
    /* Skip the 4000 bytes of audio data because the first few microphone audio data collected by the ESP32 library of Arduino have a burst sound phenomenon */
    const size_t playback_offset = min(STARTUP_SKIP_BYTES, pcm_bytes) & ~(sizeof(int16_t) - 1);
    const size_t playback_bytes = pcm_bytes - playback_offset;
    const size_t written = i2s_spk.write(
        reinterpret_cast<uint8_t *>(out_buffer) + playback_offset, playback_bytes
    );
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_DISABLE); // Disable audio power

    if (written != playback_bytes) {
        Serial.printf(
            "I2S write failed, error=%d, written=%zu/%zu\n",
            i2s_spk.lastError(), written, playback_bytes
        );
    }

    free(origin_buffer);
    free(out_buffer);
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
    if (!i2s_spk.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_BOTH)) {
        Serial.println("I2S output mode initialization failed!");
        while (1) delay(1000);
    }
}


/*---------------------------------------------------------------
 * Arduino entry points
 * The Arduino runtime calls setup() once and loop() repeatedly.
 *--------------------------------------------------------------*/
/**
 * @brief Initialize the hardware and services needed to record microphone audio and play the captured samples.
 *
 * Parameters: None.
 * @return None.
 * @note Called once by the Arduino runtime after reset.
 */
void setup() {
    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

    mic_loudspeaker_init();
}

/**
 * @brief Continue the runtime workflow used to record microphone audio and play the captured samples.
 *
 * Parameters: None.
 * @return None.
 * @note Called repeatedly by the Arduino runtime after setup() returns.
 */
void loop() {

    recording();
}
