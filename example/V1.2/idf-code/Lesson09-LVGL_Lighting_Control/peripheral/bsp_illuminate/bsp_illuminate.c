/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_illuminate.h"

/* Active LCD panel handle, shared with the LVGL port layer. */
esp_lcd_panel_handle_t panel_handle = NULL;

/* MIPI DSI bus and DBI panel I/O handles. */
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
static esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;

/* LVGL display and touch input device objects. */
static lv_display_t *my_lvgl_disp = NULL;
static lv_indev_t *my_touch_indev = NULL;

/*---------------------------------------------------------------
 * Backlight (LEDC PWM)
 *--------------------------------------------------------------*/

/**
 * @brief Configure the backlight pin as a PWM output via LEDC.
 *
 * GPIO31 is driven by LEDC timer 0 / channel 0 at 30 kHz with
 * 11-bit resolution, allowing smooth brightness control.
 *
 * @return ESP_OK on success.
 */
static esp_err_t blight_init(void)
{
    esp_err_t err = ESP_OK;

    const gpio_config_t gpio_cofig = {
        .pin_bit_mask = (1ULL << LCD_GPIO_BLIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&gpio_cofig);
    if (err != ESP_OK) {
        return err;
    }

    const ledc_timer_config_t timer_config = {
        .clk_cfg = LEDC_USE_PLL_DIV_CLK,
        .duty_resolution = LEDC_TIMER_11_BIT,
        .freq_hz = BLIGHT_PWM_Hz,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };

    const ledc_channel_config_t channel_config = {
        .gpio_num = LCD_GPIO_BLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        return err;
    }
    err = ledc_channel_config(&channel_config);
    return err;
}

/**
 * @brief Set the backlight brightness (0-100 scale).
 */
esp_err_t set_lcd_blight(uint32_t brightness)
{
    esp_err_t err = ESP_OK;

    if (brightness != 0) {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                            ((brightness * 18) + 200));
        if (err != ESP_OK) {
            return err;
        }
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    } else {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        if (err != ESP_OK) {
            return err;
        }
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    return err;
}

/*---------------------------------------------------------------
 * MIPI DSI panel bring-up
 *--------------------------------------------------------------*/

/**
 * @brief Create the MIPI DSI bus, DBI I/O and EK79007 DPI panel.
 * @return ESP_OK on success.
 */
static esp_err_t display_port_init(void)
{
    esp_err_t err = ESP_OK;
    lcd_color_rgb_pixel_format_t dpi_pixel_format;

    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = 900,
    };
    err = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
    if (err != ESP_OK) {
        return err;
    }

    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    err = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io);
    if (err != ESP_OK) {
        return err;
    }

    if (BITS_PER_PIXEL == 24) {
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB888;
    } else if (BITS_PER_PIXEL == 18) {
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB666;
    } else if (BITS_PER_PIXEL == 16) {
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    }

    const esp_lcd_dpi_panel_config_t dpi_config = {
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 51,
        .virtual_channel = 0,
        .pixel_format = dpi_pixel_format,
        .num_fbs = 1,
        .video_timing = {
            .h_size = H_size,
            .v_size = V_size,
            .hsync_back_porch = 160,
            .hsync_pulse_width = 70,
            .hsync_front_porch = 160,
            .vsync_back_porch = 23,
            .vsync_pulse_width = 10,
            .vsync_front_porch = 12,
        },
        .flags.use_dma2d = true,
    };

    ek79007_vendor_config_t vendor_config = {
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BITS_PER_PIXEL,
        .vendor_config = &vendor_config,
    };

    err = esp_lcd_new_panel_ek79007(mipi_dbi_io, &panel_config, &panel_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_lcd_panel_reset(panel_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_lcd_panel_init(panel_handle);
    return err;
}

/*---------------------------------------------------------------
 * LVGL integration (display + touch)
 *--------------------------------------------------------------*/

/**
 * @brief Start the LVGL port task, attach the DSI display and touch.
 *
 * Creates the LVGL task and registers both the MIPI DSI panel as the
 * display and the GT911 touch panel as the input device, so on-screen
 * buttons respond to finger touches.
 *
 * @return ESP_OK on success.
 */
static esp_err_t lvgl_init(void)
{
    esp_err_t err = ESP_OK;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = configMAX_PRIORITIES - 4,
        .task_stack = 8192 * 2,
        .task_affinity = -1,
        .task_max_sleep_ms = 10,
        .timer_period_ms = 5,
    };
    err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK) {
        ILLUMINATE_ERROR("LVGL port initialization failed");
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = mipi_dbi_io,
        .panel_handle = panel_handle,
        .control_handle = panel_handle,
        .buffer_size = (H_size * V_size * ((BITS_PER_PIXEL + 7) / 8)),
        .double_buffer = true,
        .hres = H_size,
        .vres = V_size,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
            .sw_rotate = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
#if CONFIG_DISPLAY_LVGL_FULL_REFRESH
            .full_refresh = true,
#else
            .full_refresh = false,
#endif
#if CONFIG_DISPLAY_LVGL_DIRECT_MODE
            .direct_mode = true,
#else
            .direct_mode = false,
#endif
        },
    };

    const lvgl_port_display_dsi_cfg_t lvgl_dpi_cfg = {
        .flags = {
#if CONFIG_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = false,
#endif
        },
    };

    my_lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &lvgl_dpi_cfg);
    if (my_lvgl_disp == NULL) {
        err = ESP_FAIL;
        ILLUMINATE_ERROR("LVGL dsi port add fail");
    }

    /* Register the GT911 touch as the LVGL pointer input device. */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = my_lvgl_disp,
        .handle = tp,
    };
    my_touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (my_touch_indev == NULL) {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL touch port add fail");
    }

    return err;
}

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Full display bring-up: backlight, DSI panel, LVGL + touch.
 *
 * Backlight is left off at the end so the caller lights it at the
 * right moment (after LVGL has drawn the first frame).
 *
 * @return ESP_OK on success.
 */
esp_err_t display_init(void)
{
    esp_err_t err = ESP_OK;

    err = blight_init();
    if (err != ESP_OK) {
        return err;
    }

    err = display_port_init();
    if (err != ESP_OK) {
        return err;
    }

    err = lvgl_init();
    if (err != ESP_OK) {
        ILLUMINATE_ERROR("Display init fail");
        return err;
    }

    set_lcd_blight(0);
    return err;
}
