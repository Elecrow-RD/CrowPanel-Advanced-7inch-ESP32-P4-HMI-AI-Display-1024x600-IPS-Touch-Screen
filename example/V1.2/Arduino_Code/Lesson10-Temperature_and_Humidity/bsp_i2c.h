/**
 * @file bsp_i2c.h
 * @brief Teaching example that demonstrates how to read a DHT20 sensor and display its measurements with LVGL.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

#ifndef _BSP_I2c_H_
#define _BSP_I2C_H_

#include "esp_log.h"           // ESP-IDF logging API.
#include "esp_err.h"           // ESP-IDF error-code definitions.
#include "driver/i2c.h"        // ESP-IDF I2C master API.
#ifdef __cplusplus 
extern "C" {
#endif  //__cplusplus

#define I2C_TAG "I2C"
#define I2C_INFO(fmt, ...) ESP_LOGI(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_DEBUG(fmt, ...) ESP_LOGD(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_ERROR(fmt, ...) ESP_LOGE(I2C_TAG, fmt, ##__VA_ARGS__)

#define I2C_MASTER_PORT ((i2c_port_t)0)  // I2C master port number (0 is default on ESP32)


/**
 * @brief Configure and install the I2C master used by the lesson peripherals.
 *
 * @param scl_io_num GPIO connected to the I2C clock line.
 * @param sda_io_num GPIO connected to the I2C data line.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called during lesson setup before the related hardware is used.
 */
esp_err_t i2c_init(int scl_io_num, int sda_io_num);
/**
 * @brief Read bytes directly from an I2C device.
 *
 * @param dev_addr Seven-bit I2C device address.
 * @param read_buffer Buffer that receives data from the I2C device.
 * @param read_size Number of bytes to read.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson needs to obtain or process new data.
 */
esp_err_t i2c_read(uint8_t dev_addr, uint8_t *read_buffer, size_t read_size);
/**
 * @brief Write bytes directly to an I2C device.
 *
 * @param dev_addr Seven-bit I2C device address.
 * @param write_buffer Buffer containing bytes to write.
 * @param write_size Number of bytes to write.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson requests the corresponding data operation.
 */
esp_err_t i2c_write(uint8_t dev_addr, uint8_t *write_buffer, size_t write_size);
/**
 * @brief Select a register, wait for the device, and read its response.
 *
 * @param dev_addr Seven-bit I2C device address.
 * @param read_reg Register address written before the read operation.
 * @param read_buffer Buffer that receives data from the I2C device.
 * @param read_size Number of bytes to read.
 * @param delayms Transaction timeout in milliseconds.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson needs to obtain or process new data.
 */
esp_err_t i2c_write_read(uint8_t dev_addr, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms);
/**
 * @brief Read bytes from a device register with a repeated-start transaction.
 *
 * @param dev_addr Seven-bit I2C device address.
 * @param reg_addr Device register address.
 * @param read_buffer Buffer that receives data from the I2C device.
 * @param read_size Number of bytes to read.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson needs to obtain or process new data.
 */
esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size);
/**
 * @brief Write one byte to a device register.
 *
 * @param dev_addr Seven-bit I2C device address.
 * @param reg_addr Device register address.
 * @param data Byte value written to the register.
 * @return ESP_OK on success; otherwise an ESP-IDF error code.
 * @note Called when the lesson requests the corresponding data operation.
 */
esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

#ifdef __cplusplus 
}
#endif  //__cplusplus
#endif