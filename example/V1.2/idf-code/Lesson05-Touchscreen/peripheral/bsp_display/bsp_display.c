/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_display.h"

/*---------------------------------------------------------------
 * GT911 touch panel state
 *--------------------------------------------------------------*/

/* Handle of the GT911 touch driver instance. */
static esp_lcd_touch_handle_t tp = NULL;
/* Panel I/O handle used by the touch driver to talk over I2C. */
static esp_lcd_panel_io_handle_t tp_io_handle = NULL;

/* Latest reported touch state, read by the application via get_coor(). */
static uint16_t touch_x = 0xffff;
static uint16_t touch_y = 0xffff;
static bool is_pressed = false;

/*---------------------------------------------------------------
 * Coordinate accessors
 *--------------------------------------------------------------*/

/**
 * @brief Update the cached touch state (internal).
 */
static void set_coor(uint16_t x, uint16_t y, bool press)
{
    touch_x = x;
    touch_y = y;
    is_pressed = press;
}

/**
 * @brief Read the latest touch coordinates and press flag.
 *
 * @param x     Output X coordinate (0xffff when not touched).
 * @param y     Output Y coordinate (0xffff when not touched).
 * @param press Output press flag.
 */
void get_coor(uint16_t *x, uint16_t *y, bool *press)
{
    *x = touch_x;
    *y = touch_y;
    *press = is_pressed;
}

/*---------------------------------------------------------------
 * GT911 initialisation
 *--------------------------------------------------------------*/

/**
 * @brief Initialise the GT911 capacitive touch panel over I2C.
 *
 * Creates the I2C panel I/O and the GT911 driver. GT911 has two
 * possible I2C addresses; if the primary one fails the backup
 * address is tried automatically.
 *
 * @return ESP_OK on success.
 */
esp_err_t touch_init(void)
{
    esp_err_t err = ESP_OK;

    /* I2C panel I/O: 16-bit commands, control phase disabled. */
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .flags = {
            .disable_control_phase = 1,
        },
        .scl_speed_hz = 400000,
    };

    /* GT911 bounds and reset/interrupt levels. */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = H_size,
        .y_max = V_size,
        .rst_gpio_num = Touch_GPIO_RST,
        .int_gpio_num = Touch_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
    };

    tp_cfg.driver_data = (void*)&io_config.dev_addr;
    /* Create the I2C panel I/O on the shared bus. */
    err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle,
                                   &io_config, &tp_io_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
    if (err != ESP_OK) {
        /* GT911 may power up on its backup address; retry with it. */
        io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
        err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle,
                                       &io_config, &tp_io_handle);
        if (err != ESP_OK) {
            return err;
        }
        err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
        if (err != ESP_OK) {
            return err;
        }
    }

    return err;
}

/*---------------------------------------------------------------
 * Touch reading
 *--------------------------------------------------------------*/

/**
 * @brief Read one sample from the touch panel and cache it.
 *
 * Fetches the raw touch data, extracts the coordinates if a finger
 * is down, and updates the cached state used by get_coor().
 *
 * @return ESP_OK on success.
 */
esp_err_t touch_read(void)
{
    esp_err_t err = ESP_OK;
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;

    /* Trigger a read of the GT911 registers. */
    err = esp_lcd_touch_read_data(tp);
    if (err != ESP_OK) {
        DISPLAY_INFO("GT911 read error");
        return err;
    }

    /* Give the driver a moment to finish copying the data. */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    /* If a finger is down, cache its coordinates. */
    if (esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 1)) {
        DISPLAY_INFO("X=%hu Y=%hu strenth=%hu cnt=%d",
                     touch_x[0], touch_y[0], touch_strength[0], touch_cnt);
        set_coor(touch_x[0], touch_y[0], true);
    } else {
        set_coor(0xffff, 0xffff, false);
    }

    return err;
}
