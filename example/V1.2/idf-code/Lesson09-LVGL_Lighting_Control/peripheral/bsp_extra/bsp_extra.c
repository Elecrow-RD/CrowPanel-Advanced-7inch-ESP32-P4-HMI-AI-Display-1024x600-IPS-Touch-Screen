/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_extra.h"

/*---------------------------------------------------------------
 * GPIO helper functions for the onboard LED (GPIO48)
 *--------------------------------------------------------------*/

/**
 * @brief Initialise GPIO48 as a digital output.
 *
 * Configures the pin as push-pull output with both pull-up and
 * pull-down disabled and no interrupt. The LED is driven directly
 * by the pin level afterwards.
 *
 * @return ESP_OK on success.
 */
esp_err_t gpio_extra_init(void)
{
    esp_err_t err = ESP_OK;

    /* Bit mask selecting GPIO48; only this pin is affected. */
    const gpio_config_t gpio_cofig = {
        .pin_bit_mask = (1ULL << 48),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };

    /* Apply the configuration to the GPIO driver. */
    err = gpio_config(&gpio_cofig);
    return ESP_OK;
}

/**
 * @brief Set the output level of the LED pin.
 *
 * @param level true drives GPIO48 high (LED on), false drives it low.
 * @return ESP_OK.
 */
esp_err_t gpio_extra_set_level(bool level)
{
    gpio_set_level(48, level);
    return ESP_OK;
}
