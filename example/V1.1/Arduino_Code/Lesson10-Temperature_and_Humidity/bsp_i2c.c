/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
static const char *TAG = "BSP_I2C";
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

esp_err_t i2c_init(int scl_io_num, int sda_io_num)
{
    esp_err_t err = ESP_OK;

    // Declare the configuration structure and assign values field by field (avoid designated initializers syntax issues)
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_io_num;          // Ensure these two macros are correctly defined
    conf.scl_io_num = scl_io_num;
    // Pull-up configuration (select according to hardware)
#ifdef CONFIG_I2C_GPIO_PULLUP
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
#else
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
#endif
    conf.master.clk_speed = 400000;           // Clock frequency 400kHz
    conf.clk_flags = 0;  

    // Configure I2C parameters
    err = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Install I2C driver (fill 0 for the last three parameters in master mode)
    err = i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized successfully");
    return ESP_OK;
}

// Read operation: Read data directly from the device
esp_err_t i2c_read(uint8_t dev_addr, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_read_from_device(I2C_MASTER_PORT, dev_addr,
                                        read_buffer, read_size, 1000 / portTICK_PERIOD_MS);
}

// Write operation: Write data directly to the device
esp_err_t i2c_write(uint8_t dev_addr, uint8_t *write_buffer, size_t write_size)
{
    return i2c_master_write_to_device(I2C_MASTER_PORT, dev_addr,
                                       write_buffer, write_size, 1000 / portTICK_PERIOD_MS);
}

// Write register address first, then read data (two independent transactions, share delayms as timeout)
esp_err_t i2c_write_read(uint8_t dev_addr, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms)
{
    // Write register address
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_PORT, dev_addr,
                                                &read_reg, 1, delayms / portTICK_PERIOD_MS);
    if (err != ESP_OK) return err;

    // Read data
    return i2c_master_read_from_device(I2C_MASTER_PORT, dev_addr,
                                        read_buffer, read_size, delayms / portTICK_PERIOD_MS);
}

// Read register: Complete writing register address and reading data in a single transaction (most commonly used method)
esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_write_read_device(I2C_MASTER_PORT, dev_addr,
                                         &reg_addr, 1,
                                         read_buffer, read_size,
                                         1000 / portTICK_PERIOD_MS);
}

// Write register: Write register address + data
esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_PORT, dev_addr,
                                       write_buf, sizeof(write_buf),
                                       1000 / portTICK_PERIOD_MS);
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/