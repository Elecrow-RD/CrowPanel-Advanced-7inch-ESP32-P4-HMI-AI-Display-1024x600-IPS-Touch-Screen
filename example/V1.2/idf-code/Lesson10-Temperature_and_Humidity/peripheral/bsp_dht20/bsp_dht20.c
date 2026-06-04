/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_dht20.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

static i2c_master_dev_handle_t dht20_handle = NULL;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

char *print_binary(uint16_t value)
{
    static char binary_str[17]; // 16 bits + null-terminator
    binary_str[16] = '\0';      // Null-terminate the string
    for (int i = 15; i >= 0; i--)
    {
        binary_str[15 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    return binary_str;
}

const char *bit_rep[16] = {
    [0] = "0000",
    [1] = "0001",
    [2] = "0010",
    [3] = "0011",
    [4] = "0100",
    [5] = "0101",
    [6] = "0110",
    [7] = "0111",
    [8] = "1000",
    [9] = "1001",
    [10] = "1010",
    [11] = "1011",
    [12] = "1100",
    [13] = "1101",
    [14] = "1110",
    [15] = "1111",
};

char *print_byte(uint8_t byte)
{
    static char binbyte[11];
    sprintf(binbyte, "0b%s %s", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
    return binbyte;
}

static esp_err_t dht20_reset_register(uint8_t reg)
{
    esp_err_t err = ESP_OK;
    static uint8_t values[3] = {0};
    uint8_t txbuffer[3] = {reg, 0x00, 0x00};    /*Register address*/
    err = i2c_write(dht20_handle, txbuffer, 3); /*I2C write command*/
    if (err != ESP_OK)
        return err;
    vTaskDelay(5 / portTICK_PERIOD_MS);
    err = i2c_read(dht20_handle, values, 3); /*I2C read status*/
    if (err != ESP_OK)
        return err;
    vTaskDelay(10 / portTICK_PERIOD_MS);

    memset(txbuffer, 0, sizeof(txbuffer));
    txbuffer[0] = (0xB0 | reg);
    txbuffer[1] = values[1];
    txbuffer[2] = values[2];

    err = i2c_write(dht20_handle, txbuffer, 3);
    if (err != ESP_OK)
        return err;
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return err;
}

uint8_t dht20_status(void)
{
    esp_err_t err = ESP_OK;
    static uint8_t txbuf = 0x71;
    static uint8_t rxdata;
    err = i2c_write(dht20_handle, &txbuf, 1); /*I2C write command*/
    if (err != ESP_OK)
        return err;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    err = i2c_read(dht20_handle, &rxdata, 1); /*I2C read status*/
    if (err != ESP_OK)
        return err;
    return (rxdata);
}

static uint8_t dht20_reset_sensor(void)
{
    static uint8_t rst_count = 0;
    uint8_t status = dht20_status(); /*Obtain the status of the DHT20 sensor*/
    DHT20_DEBUG("Sensor status: %s - 0x%02X", print_byte(status), status);
    while ((status & 0x18) != 0x18) /*If the DHT20 sensor status code is not 0x18*/
    {
        DHT20_DEBUG("Sensor status: %s - 0x%02X", print_byte(status), status);
        rst_count++;
        if (dht20_reset_register(0x1B) != ESP_OK) /*DHT20 sensor register reset function*/
        {
            rst_count++;
        }
        if (dht20_reset_register(0x1C) != ESP_OK)
        {
            rst_count++;
        }
        if (dht20_reset_register(0x1E) != ESP_OK)
        {
            rst_count++;
        }
        if (rst_count >= 255) /*Exceed the retry count*/
            return rst_count;
        DHT20_DEBUG("Registers resetted [%d] times!", rst_count);
        status = dht20_status(); /*Obtain the status of the DHT20 sensor*/
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    return rst_count;
}

esp_err_t dht20_begin(void)
{
    esp_err_t err = ESP_OK;
    dht20_handle = i2c_dev_register(DHT20_I2C_ADDRESS); /*DHT20 sensor address registration*/
    if (dht20_handle != NULL)
    {
        if (dht20_reset_sensor() >= 255) /* Check status register, if status not eq 0x18 reset the necearry registers*/
            err = ESP_FAIL;
    }
    else
    {
        err = ESP_FAIL;
        return err;
    }
    return err;
}

esp_err_t dht20_is_calibrated(void)
{
    esp_err_t err = ESP_OK;
    uint8_t status_byte = dht20_status();
    if ((status_byte & 0x18) != 0x18) /*If the DHT20 sensor status code is not 0x18*/
    {
        err = ESP_FAIL;
    }
    return err;
}

static uint8_t dht20_crc8(uint8_t *message, uint8_t Num)
{
    static uint8_t i;
    static uint8_t byte;
    uint8_t crc = 0xFF;
    for (byte = 0; byte < Num; byte++)
    {
        crc ^= (message[byte]);
        for (i = 8; i > 0; --i)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31; /*Calculation of CRC8 Checksum for DHT20 Data*/
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

esp_err_t dht20_read_data(dht20_data_t *data)
{
    esp_err_t err = ESP_OK;
    data->humidity = 0.0f;
    data->raw_humid = 0;
    data->temperature = 0.0f;
    data->raw_temp = 0;
    static uint8_t txbuf[3] = {0xAC, 0x33, 0x00}; /*Start measuring command + 2 bytes as parameters*/
    static uint8_t status_byte[1] = {0};          /*retrieve measuring status*/
    static uint8_t rxdata[7] = {0};               /*retrieve measurement data (temp + humid)*/
    err = i2c_write(dht20_handle, txbuf, 3);
    if (err != ESP_OK)
        return err;
    vTaskDelay(80 / portTICK_PERIOD_MS); /*minimum delay as per datahseet*/
    DHT20_DEBUG("Reading registers.....");
    err = i2c_read(dht20_handle, status_byte, 1); /*read status byte to check sensor is done measuring*/
    if (err != ESP_OK)
        return err;
    unsigned long start_time = esp_timer_get_time() / 1000;
    while ((status_byte[0] >> 7) != 0) /*while meassurment is ongoing*/
    {
        if ((esp_timer_get_time() / 1000) - start_time >= DHT20_MEASURE_TIMEOUT)
        {
            return ESP_ERR_TIMEOUT; /*Timeout*/
        }
        portYIELD(); /*give scheduler some time for other tasks while we're still measuring*/
    }
    err = i2c_read(dht20_handle, rxdata, 7); /*sensor idle let's read all data*/
    if (err != ESP_OK)
        return err;
    DHT20_DEBUG("Byte1: %s", print_byte(rxdata[0]));
    DHT20_DEBUG("Byte2: %s", print_byte(rxdata[1]));
    DHT20_DEBUG("Byte3: %s", print_byte(rxdata[2]));
    DHT20_DEBUG("Byte4: %s", print_byte(rxdata[3]));
    DHT20_DEBUG("Byte5: %s", print_byte(rxdata[4]));
    DHT20_DEBUG("Byte6: %s", print_byte(rxdata[5]));
    DHT20_DEBUG("CRC Byte: %s", print_byte(rxdata[6]));

    uint8_t get_crc = dht20_crc8(rxdata, 6); /*CRC calculation*/
    DHT20_DEBUG("Data byte 7: 0x%02X, calculated crc8: 0x%02X", rxdata[6], get_crc);

    if (rxdata[6] == get_crc) /*Compare CRC calues and continue if they match*/
    {
        uint32_t raw_humid = rxdata[1];
        raw_humid <<= 8;
        raw_humid += rxdata[2];
        raw_humid <<= 4;
        raw_humid += rxdata[3] >> 4;
        data->raw_humid = raw_humid;
        data->humidity = (float)(raw_humid / 1048576.0f) * 100.0f; /*convert RAW to Humidity in %*/

        DHT20_DEBUG("Humidity raw: %lu - Converted: %.1f %%", data->raw_humid, data->humidity);

        uint32_t raw_temp = (rxdata[3] & 0x0F);
        raw_temp <<= 8;
        raw_temp += rxdata[4];
        raw_temp <<= 8;
        raw_temp += rxdata[5];
        data->raw_temp = raw_temp;
        data->temperature = (float)(raw_temp / 1048576.0f) * 200.0f - 50.0f; /*convert RAW to Celsius C*/

        DHT20_DEBUG("Temperature raw: %lu - Converted: %.2fC.", data->raw_temp, data->temperature);
    }
    else
    {
        DHT20_ERROR("CRC Checksum failed !!!");
        return ESP_ERR_INVALID_CRC;
    }
    return 0.0f;
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/