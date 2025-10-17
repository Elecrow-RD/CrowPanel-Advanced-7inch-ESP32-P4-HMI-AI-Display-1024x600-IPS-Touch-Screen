#ifndef _BSP_I2c_H_
#define _BSP_I2C_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"           //References for LOG Printing Function-related API Functions
#include "esp_err.h"           //References for Error Type Function-related API Functions
#include "driver/i2c_master.h" //References for I2C Master Function-related API Functions
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define I2C_TAG "I2C"
#define I2C_INFO(fmt, ...) ESP_LOGI(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_DEBUG(fmt, ...) ESP_LOGD(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_ERROR(fmt, ...) ESP_LOGE(I2C_TAG, fmt, ##__VA_ARGS__)

#define I2C_MASTER_PORT 0  // I2C master port number (0 is default on ESP32)
#define I2C_GPIO_SCL 46    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA 45    // GPIO number used for I2C SDA (data) line

extern i2c_master_bus_handle_t i2c_bus_handle; // Global handle for I2C bus

// Function declarations for I2C operations
esp_err_t i2c_init(void);  // Initialize I2C master bus
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address); // Register an I2C device with its address
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size); // Read bytes from I2C device
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size); // Write bytes to I2C device
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms); // Write register address, then read data
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size); // Read specific register
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data); // Write data to specific register

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif