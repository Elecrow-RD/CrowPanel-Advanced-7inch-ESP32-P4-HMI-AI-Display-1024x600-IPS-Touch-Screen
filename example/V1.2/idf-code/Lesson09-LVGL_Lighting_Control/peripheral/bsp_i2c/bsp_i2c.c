/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

i2c_master_bus_handle_t i2c_bus_handle = NULL;

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

esp_err_t i2c_init(void)
{
    static esp_err_t err = ESP_OK;
    /*I2C master bus specific configurations*/
    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MASTER_PORT,       /* I2C port number*/
        .sda_io_num = I2C_GPIO_SDA,        /* GPIO number of I2C SDA signal*/
        .scl_io_num = I2C_GPIO_SCL,        /*GPIO number of I2C SCL signal*/
        .clk_source = I2C_CLK_SRC_DEFAULT, /*Clock source of I2C master bus*/
        .glitch_ignore_cnt = 7,            /* If the glitch period on the line is less than this value, it can be filtered out, typically value is 7*/
        .flags.enable_internal_pullup = true, /*Enable internal pullups*/
    };
    err = i2c_new_master_bus(&conf, &i2c_bus_handle); /*Allocate an I2C master bus*/
    if (err != ESP_OK)
        return err;
    return err;
}

i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address)
{
    esp_err_t err = ESP_OK;
    i2c_master_dev_handle_t dev_handle = NULL; /*Type of I2C master bus device handle*/
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, /*Select the address length of the slave device*/
        .device_address = dev_device_address,  /*I2C device raw address. (The 7/10 bit address without read/write bit)*/
        .scl_speed_hz = 400000,                /*I2C SCL line frequency*/
    };
    err = i2c_master_bus_add_device(i2c_bus_handle, &cfg, &dev_handle); /*Add I2C master BUS device*/
    if (err == ESP_OK)
        return dev_handle;
    return 0;
}

esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_receive(i2c_dev, read_buffer, read_size, 1000); /*Perform a read transaction on the I2C bus*/
}

esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size)
{
    return i2c_master_transmit(i2c_dev, write_buffer, write_size, 1000); /*Perform a write transaction on the I2C bus*/
}

esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms)
{
    esp_err_t err = ESP_OK;
    err = i2c_master_transmit(i2c_dev, &read_reg, 1, delayms); /*Perform a write transaction on the I2C bus*/
    if (err != ESP_OK)
        return err;
    err = i2c_master_receive(i2c_dev, read_buffer, read_size, delayms); /*Perform a read transaction on the I2C bus*/
    if (err != ESP_OK)
        return err;
    return err;
}

esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_transmit_receive(i2c_dev, &reg_addr, 1, read_buffer, read_size, 1000); /*Perform a write-read transaction on the I2C bus*/
}

esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};                                 /*Register address and data*/
    return i2c_master_transmit(i2c_dev, write_buf, sizeof(write_buf), 1000); /*Perform a write transaction on the I2C bus*/
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/