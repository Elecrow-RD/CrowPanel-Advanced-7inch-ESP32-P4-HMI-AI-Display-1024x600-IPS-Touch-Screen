#ifndef _BSP_AUDIO_H_  // Prevent multiple inclusion of this header file
#define _BSP_AUDIO_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>           // Include string manipulation functions
#include <stdint.h>           // Include standard integer type definitions
#include "freertos/FreeRTOS.h" // Include FreeRTOS core definitions
#include "freertos/task.h"    // Include FreeRTOS task management functions
#include "esp_log.h"          // Include ESP-IDF logging utilities
#include "esp_err.h"          // Include ESP-IDF error handling definitions
#include "driver/gpio.h"      // Include GPIO driver functions
#include "driver/i2s_std.h"   // Include standard I2S driver API
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define AUDIO_TAG "AUDIO"  // Define the logging tag for audio module
#define AUDIO_INFO(fmt, ...)    ESP_LOGI(AUDIO_TAG, fmt, ##__VA_ARGS__)  // Info-level log macro for audio module
#define AUDIO_DEBUG(fmt, ...)   ESP_LOGD(AUDIO_TAG, fmt, ##__VA_ARGS__)  // Debug-level log macro for audio module
#define AUDIO_ERROR(fmt, ...)   ESP_LOGE(AUDIO_TAG, fmt, ##__VA_ARGS__)  // Error-level log macro for audio module

#define AUDIO_GPIO_LRCLK    21   // GPIO pin number for LRCLK (Left-Right Clock)
#define AUDIO_GPIO_BCLK     22   // GPIO pin number for BCLK (Bit Clock)
#define AUDIO_GPIO_SDATA    23   // GPIO pin number for SDATA (Serial Data)
#define AUDIO_GPIO_CTRL     30   // GPIO pin number for audio amplifier control

esp_err_t audio_init();            // Function prototype for audio initialization
esp_err_t audio_ctrl_init();       // Function prototype for audio amplifier control initialization
esp_err_t set_Audio_ctrl(bool state);  // Function prototype to enable or disable the audio amplifier
i2s_chan_handle_t get_audio_handle();  // Function prototype to get the I2S channel handle

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif  // End of include guard
