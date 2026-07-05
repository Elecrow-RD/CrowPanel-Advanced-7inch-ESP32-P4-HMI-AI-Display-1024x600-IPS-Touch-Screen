#ifndef _BSP_I2c_H_
#define _BSP_I2C_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <rom/ets_sys.h>
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define I2C_TAG "I2C"
#define I2C_INFO(fmt, ...) ESP_LOGI(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_DEBUG(fmt, ...) ESP_LOGD(I2C_TAG, fmt, ##__VA_ARGS__)
#define I2C_ERROR(fmt, ...) ESP_LOGE(I2C_TAG, fmt, ##__VA_ARGS__)

#ifdef CONFIG_BSP_I2C_ENABLED

#define I2C_MASTER_PORT CONFIG_I2C_PORT_NUM
#define I2C_GPIO_SCL CONFIG_I2C_GPIO_SCL
#define I2C_GPIO_SDA CONFIG_I2C_GPIO_SDA

esp_err_t i2c_init(void);
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address);
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size);
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size);
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms);
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size);
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data);

char *print_binary(uint16_t value);
char *print_byte(uint8_t byte);

extern i2c_master_bus_handle_t i2c_bus_handle;
#endif
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif