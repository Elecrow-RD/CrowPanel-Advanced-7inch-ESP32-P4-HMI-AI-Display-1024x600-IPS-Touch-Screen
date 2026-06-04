#ifndef _BSP_I2c_H_       // Header guard start: prevent multiple inclusion of this file
#define _BSP_I2C_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <stdio.h>         // Standard I/O functions
#include <stdint.h>        // Standard integer types (e.g., uint8_t, uint16_t)
#include <stdbool.h>       // Boolean type (true/false)
#include <rom/ets_sys.h>   // ESP ROM system functions (e.g., delay)
#include "esp_timer.h"     // ESP-IDF high-resolution timer functions
#include "driver/i2c_master.h" // ESP-IDF I2C master driver API
#include "esp_log.h"       // ESP-IDF logging functions
#include "esp_err.h"       // ESP-IDF error codes and handling
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define I2C_TAG "I2C"      // Log tag for identifying I2C logs
#define I2C_INFO(fmt, ...) ESP_LOGI(I2C_TAG, fmt, ##__VA_ARGS__)  // Info log macro
#define I2C_DEBUG(fmt, ...) ESP_LOGD(I2C_TAG, fmt, ##__VA_ARGS__) // Debug log macro
#define I2C_ERROR(fmt, ...) ESP_LOGE(I2C_TAG, fmt, ##__VA_ARGS__) // Error log macro

#define I2C_MASTER_PORT 0  // I2C master port number (0 is default on ESP32)
#define I2C_GPIO_SCL 46    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA 45    // GPIO number used for I2C SDA (data) line

// Function declarations for I2C operations
esp_err_t i2c_init(void);  // Initialize I2C master bus
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address); // Register an I2C device with its address
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size); // Read bytes from I2C device
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size); // Write bytes to I2C device
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms); // Write register address, then read data
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size); // Read specific register
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data); // Write data to specific register

// Utility functions
char *print_binary(uint16_t value); // Convert 16-bit integer to binary string
char *print_byte(uint8_t byte);     // Convert 8-bit integer to binary string with formatting

extern i2c_master_bus_handle_t i2c_bus_handle; // Global handle for I2C bus
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif  // End of header guard
