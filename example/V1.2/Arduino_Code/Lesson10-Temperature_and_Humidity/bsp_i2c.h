#ifndef _BSP_I2c_H_
#define _BSP_I2C_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"           //References for LOG Printing Function-related API Functions
#include "esp_err.h"           //References for Error Type Function-related API Functions
#include "driver/i2c.h"        //References for I2C Master Function-related API Functions
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef __cplusplus 
extern "C" {
#endif  //__cplusplus

#define I2C_TAG "I2C"
#define I2C_INFO(fmt, ...) ESP_LOGI(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_DEBUG(fmt, ...) ESP_LOGD(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_ERROR(fmt, ...) ESP_LOGE(I2C_TAG, fmt, ##__VA_ARGS__)

#define I2C_MASTER_PORT ((i2c_port_t)0)  // I2C master port number (0 is default on ESP32)


// Function declarations for I2C operations
esp_err_t i2c_init(int scl_io_num, int sda_io_num);
esp_err_t i2c_read(uint8_t dev_addr, uint8_t *read_buffer, size_t read_size);
esp_err_t i2c_write(uint8_t dev_addr, uint8_t *write_buffer, size_t write_size);
esp_err_t i2c_write_read(uint8_t dev_addr, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms);
esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size);
esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

#ifdef __cplusplus 
}
#endif  //__cplusplus
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif