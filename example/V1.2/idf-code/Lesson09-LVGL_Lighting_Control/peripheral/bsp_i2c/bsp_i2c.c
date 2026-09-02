/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_i2c.h"

/* Global handle of the I2C master bus, shared with display/touch code. */
i2c_master_bus_handle_t i2c_bus_handle = NULL;

/*---------------------------------------------------------------
 * I2C bus and device management
 *--------------------------------------------------------------*/

/**
 * @brief Create the I2C master bus on the configured SDA/SCL pins.
 *
 * Uses internal pull-ups and a glitch filter of 7 cycles.
 *
 * @return ESP_OK on success, an error code otherwise.
 */
esp_err_t i2c_init(void)
{
    static esp_err_t err = ESP_OK;

    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_GPIO_SDA,
        .scl_io_num = I2C_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    err = i2c_new_master_bus(&conf, &i2c_bus_handle);
    return err;
}

/**
 * @brief Register a device on the I2C bus at 400 kHz, 7-bit address.
 *
 * @param dev_device_address 7-bit I2C address of the target device.
 * @return Device handle on success, NULL (0) on failure.
 */
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address)
{
    i2c_master_dev_handle_t dev_handle = NULL;

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_device_address,
        .scl_speed_hz = 400000,
    };

    esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &cfg, &dev_handle);
    if (err == ESP_OK) {
        return dev_handle;
    }
    return 0;
}

/*---------------------------------------------------------------
 * Single-shot I2C transfers
 *--------------------------------------------------------------*/

/**
 * @brief Read raw bytes from a device (no register prefix).
 */
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_receive(i2c_dev, read_buffer, read_size, 1000);
}

/**
 * @brief Write raw bytes to a device (no register prefix).
 */
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size)
{
    return i2c_master_transmit(i2c_dev, write_buffer, write_size, 1000);
}

/**
 * @brief Write a register address then read back data (two transfers).
 */
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg,
                         uint8_t *read_buffer, size_t read_size, uint16_t delayms)
{
    esp_err_t err = ESP_OK;
    err = i2c_master_transmit(i2c_dev, &read_reg, 1, delayms);
    if (err != ESP_OK) {
        return err;
    }
    err = i2c_master_receive(i2c_dev, read_buffer, read_size, delayms);
    return err;
}

/**
 * @brief Read from a specific register using the combined transmit-receive.
 */
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr,
                       uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_transmit_receive(i2c_dev, &reg_addr, 1, read_buffer, read_size, 1000);
}

/**
 * @brief Write one byte to a specific register.
 */
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(i2c_dev, write_buf, sizeof(write_buf), 1000);
}
