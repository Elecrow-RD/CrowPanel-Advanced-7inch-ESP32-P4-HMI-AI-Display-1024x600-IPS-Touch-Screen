/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_extra.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

esp_err_t gpio_extra_init()                    // Function to initialize GPIO48 as output
{
    esp_err_t err = ESP_OK;                    // Variable to store error code, initialized to ESP_OK
    const gpio_config_t gpio_cofig = {         // Define GPIO configuration structure
        .pin_bit_mask = (1ULL << 48),          // Select GPIO48 by setting bit 48 in the mask
        .mode = GPIO_MODE_OUTPUT,              // Configure GPIO48 as output mode
        .pull_up_en = false,                   // Disable internal pull-up resistor
        .pull_down_en = false,                 // Disable internal pull-down resistor
        .intr_type = GPIO_INTR_DISABLE,        // Disable GPIO interrupt for this pin
    };
    err = gpio_config(&gpio_cofig);            // Apply the configuration using ESP-IDF API
    return ESP_OK;                             // Always return ESP_OK (ignores actual error code)
}

esp_err_t gpio_extra_set_level(bool level)     // Function to set output level of GPIO48
{
    gpio_set_level(48, level);                 // Set GPIO48 output to high (1) or low (0) depending on 'level'
    return ESP_OK;                             // Return ESP_OK (does not check for errors)
}

