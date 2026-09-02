/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_dht20.h"

/* I2C device handle for the DHT20 sensor. */
static i2c_master_dev_handle_t dht20_handle = NULL;

/*---------------------------------------------------------------
 * Debug helpers
 *--------------------------------------------------------------*/

/**
 * @brief Render a 16-bit value as a binary string.
 */
char *print_binary(uint16_t value)
{
    static char binary_str[17];
    binary_str[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        binary_str[15 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    return binary_str;
}

/* Lookup table of 4-bit patterns indexed by hex digit 0..15. */
const char *bit_rep[16] = {
    [0]  = "0000", [1]  = "0001", [2]  = "0010", [3]  = "0011",
    [4]  = "0100", [5]  = "0101", [6]  = "0110", [7]  = "0111",
    [8]  = "1000", [9]  = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

/**
 * @brief Render a byte as "0bHHHH LLLL" for debug prints.
 */
char *print_byte(uint8_t byte)
{
    static char binbyte[11];
    sprintf(binbyte, "0b%s %s", bit_rep[byte >> 4], bit_rep[byte & 0x0F]);
    return binbyte;
}

/*---------------------------------------------------------------
 * Register reset / status
 *--------------------------------------------------------------*/

/**
 * @brief Reset one of the DHT20 calibration registers.
 *
 * Reads the current register content and writes it back with the
 * reset command prefix (0xB0 | reg) to restore factory calibration.
 *
 * @param reg Register address to reset (0x1B / 0x1C / 0x1E).
 * @return ESP_OK on success.
 */
static esp_err_t dht20_reset_register(uint8_t reg)
{
    esp_err_t err = ESP_OK;
    static uint8_t values[3] = {0};

    /* Read the current register content. */
    uint8_t txbuffer[3] = {reg, 0x00, 0x00};
    err = i2c_write(dht20_handle, txbuffer, 3);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);
    err = i2c_read(dht20_handle, values, 3);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* Write the values back with the reset command prefix. */
    memset(txbuffer, 0, sizeof(txbuffer));
    txbuffer[0] = (0xB0 | reg);
    txbuffer[1] = values[1];
    txbuffer[2] = values[2];

    err = i2c_write(dht20_handle, txbuffer, 3);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(5 / portTICK_PERIOD_MS);
    return err;
}

/**
 * @brief Read the DHT20 status byte (0x71).
 * @return The raw status byte, or 0 on I2C error.
 */
uint8_t dht20_status(void)
{
    esp_err_t err = ESP_OK;
    static uint8_t txbuf = 0x71;
    static uint8_t rxdata;
    err = i2c_write(dht20_handle, &txbuf, 1);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    err = i2c_read(dht20_handle, &rxdata, 1);
    if (err != ESP_OK) {
        return err;
    }
    return rxdata;
}

/**
 * @brief Reset the sensor until the status byte indicates calibrated.
 *
 * The sensor is ready when bits 3 and 4 of the status byte are both
 * set (0x18). If not, registers 0x1B/0x1C/0x1E are reset and the
 * status is re-read, up to 255 attempts.
 *
 * @return Number of reset attempts (>= 255 means it gave up).
 */
static uint8_t dht20_reset_sensor(void)
{
    static uint8_t rst_count = 0;
    uint8_t status = dht20_status();
    DHT20_DEBUG("Sensor status: %s - 0x%02X", print_byte(status), status);

    /* Loop while the sensor reports it is not calibrated. */
    while ((status & 0x18) != 0x18) {
        DHT20_DEBUG("Sensor status: %s - 0x%02X", print_byte(status), status);
        rst_count++;
        if (dht20_reset_register(0x1B) != ESP_OK) {
            rst_count++;
        }
        if (dht20_reset_register(0x1C) != ESP_OK) {
            rst_count++;
        }
        if (dht20_reset_register(0x1E) != ESP_OK) {
            rst_count++;
        }
        if (rst_count >= 255) {
            return rst_count;
        }
        DHT20_DEBUG("Registers resetted [%d] times!", rst_count);
        status = dht20_status();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    return rst_count;
}

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the DHT20 sensor.
 *
 * Registers the I2C device and resets the sensor until it reports
 * calibrated status.
 *
 * @return ESP_OK on success, ESP_FAIL on registration or reset failure.
 */
esp_err_t dht20_begin(void)
{
    esp_err_t err = ESP_OK;
    dht20_handle = i2c_dev_register(DHT20_I2C_ADDRESS);
    if (dht20_handle != NULL) {
        if (dht20_reset_sensor() >= 255) {
            err = ESP_FAIL;
        }
    } else {
        err = ESP_FAIL;
        return err;
    }
    return err;
}

/**
 * @brief Check whether the sensor reports calibrated status.
 * @return ESP_OK if calibrated, ESP_FAIL otherwise.
 */
esp_err_t dht20_is_calibrated(void)
{
    esp_err_t err = ESP_OK;
    uint8_t status_byte = dht20_status();
    if ((status_byte & 0x18) != 0x18) {
        err = ESP_FAIL;
    }
    return err;
}

/**
 * @brief Compute the DHT20 CRC-8 (polynomial 0x31) of a buffer.
 * @param message Data bytes.
 * @param Num     Number of bytes.
 * @return Computed CRC byte.
 */
static uint8_t dht20_crc8(uint8_t *message, uint8_t Num)
{
    static uint8_t i;
    static uint8_t byte;
    uint8_t crc = 0xFF;
    for (byte = 0; byte < Num; byte++) {
        crc ^= message[byte];
        for (i = 8; i > 0; --i) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

/**
 * @brief Trigger a measurement and read temperature/humidity.
 *
 * Sends the measure command (0xAC 0x33 0x00), waits for the busy
 * bit to clear (with a timeout), reads 7 bytes (status + 5 data +
 * CRC), verifies the CRC and converts the raw values to degrees
 * Celsius and percent RH.
 *
 * @param data Output structure receiving the converted values.
 * @return ESP_OK on success, ESP_ERR_TIMEOUT or ESP_ERR_INVALID_CRC on failure.
 */
esp_err_t dht20_read_data(dht20_data_t *data)
{
    esp_err_t err = ESP_OK;
    data->humidity = 0.0f;
    data->raw_humid = 0;
    data->temperature = 0.0f;
    data->raw_temp = 0;

    /* Measure command + two parameter bytes. */
    static uint8_t txbuf[3] = {0xAC, 0x33, 0x00};
    static uint8_t status_byte[1] = {0};
    static uint8_t rxdata[7] = {0};

    err = i2c_write(dht20_handle, txbuf, 3);
    if (err != ESP_OK) {
        return err;
    }
    /* The sensor needs at least 80 ms to finish measuring. */
    vTaskDelay(80 / portTICK_PERIOD_MS);
    DHT20_DEBUG("Reading registers.....");

    err = i2c_read(dht20_handle, status_byte, 1);
    if (err != ESP_OK) {
        return err;
    }

    /* Wait while the busy bit (bit 7) is set. */
    unsigned long start_time = esp_timer_get_time() / 1000;
    while ((status_byte[0] >> 7) != 0) {
        if ((esp_timer_get_time() / 1000) - start_time >= DHT20_MEASURE_TIMEOUT) {
            return ESP_ERR_TIMEOUT;
        }
        portYIELD();
    }

    /* Read the 7-byte result: status + 5 data + CRC. */
    err = i2c_read(dht20_handle, rxdata, 7);
    if (err != ESP_OK) {
        return err;
    }

    DHT20_DEBUG("Byte1: %s", print_byte(rxdata[0]));
    DHT20_DEBUG("Byte2: %s", print_byte(rxdata[1]));
    DHT20_DEBUG("Byte3: %s", print_byte(rxdata[2]));
    DHT20_DEBUG("Byte4: %s", print_byte(rxdata[3]));
    DHT20_DEBUG("Byte5: %s", print_byte(rxdata[4]));
    DHT20_DEBUG("Byte6: %s", print_byte(rxdata[5]));
    DHT20_DEBUG("CRC Byte: %s", print_byte(rxdata[6]));

    /* Verify the CRC over the first 6 bytes. */
    uint8_t get_crc = dht20_crc8(rxdata, 6);
    DHT20_DEBUG("Data byte 7: 0x%02X, calculated crc8: 0x%02X", rxdata[6], get_crc);

    if (rxdata[6] == get_crc) {
        /* Humidity: 20-bit value spread across bytes 1-3 (high nibble of byte 3). */
        uint32_t raw_humid = rxdata[1];
        raw_humid <<= 8;
        raw_humid += rxdata[2];
        raw_humid <<= 4;
        raw_humid += rxdata[3] >> 4;
        data->raw_humid = raw_humid;
        data->humidity = (float)(raw_humid / 1048576.0f) * 100.0f;

        DHT20_DEBUG("Humidity raw: %lu - Converted: %.1f %%", data->raw_humid, data->humidity);

        /* Temperature: 20-bit value spread across bytes 3-5 (low nibble of byte 3). */
        uint32_t raw_temp = (rxdata[3] & 0x0F);
        raw_temp <<= 8;
        raw_temp += rxdata[4];
        raw_temp <<= 8;
        raw_temp += rxdata[5];
        data->raw_temp = raw_temp;
        data->temperature = (float)(raw_temp / 1048576.0f) * 200.0f - 50.0f;

        DHT20_DEBUG("Temperature raw: %lu - Converted: %.2fC.", data->raw_temp, data->temperature);
    } else {
        DHT20_ERROR("CRC Checksum failed !!!");
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}
