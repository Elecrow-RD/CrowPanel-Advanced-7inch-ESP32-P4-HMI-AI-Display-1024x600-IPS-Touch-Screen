#ifndef _BSP_EXTRA_H_
#define _BSP_EXTRA_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

#include <string.h>      // Standard C library for string handling functions
#include <stdint.h>      // Standard C library for fixed-width integer types
#include "esp_log.h"     // ESP-IDF logging library for debug/info/error logs
#include "esp_err.h"     // ESP-IDF error code definitions and handling utilities
#include "driver/gpio.h" // ESP-IDF GPIO driver for configuring and controlling pins

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

#define EXTRA_TAG "EXTRA"                           // Define log tag name "EXTRA" used for identifying log messages
#define EXTRA_INFO(fmt, ...) ESP_LOGI(EXTRA_TAG, fmt, ##__VA_ARGS__)   // Macro for info-level logging with tag "EXTRA"
#define EXTRA_DEBUG(fmt, ...) ESP_LOGD(EXTRA_TAG, fmt, ##__VA_ARGS__)  // Macro for debug-level logging with tag "EXTRA"
#define EXTRA_ERROR(fmt, ...) ESP_LOGE(EXTRA_TAG, fmt, ##__VA_ARGS__)  // Macro for error-level logging with tag "EXTRA"

esp_err_t gpio_extra_init();                        // Function declaration for initializing GPIO
esp_err_t gpio_extra_set_level(bool level);         // Function declaration for setting GPIO output level (high/low)

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif