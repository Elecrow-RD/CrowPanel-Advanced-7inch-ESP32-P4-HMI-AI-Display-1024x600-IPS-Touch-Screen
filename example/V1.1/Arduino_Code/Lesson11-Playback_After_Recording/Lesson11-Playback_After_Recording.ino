/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs

#include <string.h>         // Include standard string manipulation functions
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes

#include <ESP_I2S.h>        // ESP32 I2S Library
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

#define MAIN_INFO(fmt, ...)         PRINTF_INFO(fmt, ##__VA_ARGS__)   // Info level log macro
#define MAIN_ERROR(fmt, ...)        PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Error level log macro
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/*microphone*/
static I2SClass i2s_mic; // Create an I2SClass microphone instance
/*loudspeaker*/
static I2SClass i2s_spk; // Create an I2SClass speaker instance

static TaskHandle_t mic_task_handle = NULL;

void recording()
{
    uint8_t *origin_buffer;
    int16_t *read_buffer;
    size_t read_bytes;
    int16_t *out_buffer;
    out_buffer = (int16_t*)heap_caps_malloc((16000 * 2 * 5), MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
    if (out_buffer==NULL) {
        Serial.println("Memory allocation failure!");
    }
    
    Serial.println("Recording 5 seconds of audio data...");
    origin_buffer = i2s_mic.recordWAV(5, &read_bytes);
    read_bytes -= 44;
    read_buffer = (int16_t*)(44 + origin_buffer);
    size_t num_samples = read_bytes / 2; // Number of samples
    
    Serial.printf("read_bytes = %d\n", read_bytes);
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
    i2s_spk.write((uint8_t*)out_buffer + 4000, read_bytes - 4000);
    digitalWrite(AUDIO_GPIO_CTRL, AUDIO_POWER_DISABLE); // Disable audio power

    free(origin_buffer);

    free(out_buffer);
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
    if (!i2s_spk.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_BOTH)) {
        Serial.println("I2S output mode initialization failed!");
        while (1) delay(1000);
    }
}


void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

    mic_loudspeaker_init();
}

void loop() {
    // put your main code here, to run repeatedly:

    recording();
}