/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_display.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef CONFIG_BSP_DISPLAY_ENABLED
esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
static esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
static lv_display_t *my_lvgl_disp = NULL;
static lv_obj_t *color_obj;
lv_color_t *color_buffer;
void *display_buffer = NULL;

#ifdef CONFIG_BSP_TOUCH_ENABLED
static esp_lcd_touch_handle_t tp = NULL;
static esp_lcd_panel_io_handle_t tp_io_handle = NULL;
static lv_indev_t *my_touch_indev;
#endif

#endif

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
#ifdef CONFIG_BSP_DISPLAY_ENABLED

#ifdef CONFIG_BSP_TOUCH_ENABLED

static lv_obj_t *touch_obj = NULL;
void set_touch_display(bool state)
{
    if (state)
    {
        if (touch_obj != NULL)
            lv_obj_clear_flag(touch_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (touch_obj != NULL)
            lv_obj_add_flag(touch_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

esp_err_t touch_init(void)
{
    esp_err_t err = ESP_OK;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .flags =
            {
                .disable_control_phase = 1,
            },
        .scl_speed_hz = 400000,
    };
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
    err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle, &io_config, &tp_io_handle);
    if (err != ESP_OK)
        return err;
    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
    return err;
}

static uint16_t touch_x = 0xffff;
static uint16_t touch_y = 0xffff;
static bool is_pressed = false; 

static void set_coor(uint16_t x, uint16_t y, bool press)
{
    touch_x = x;
    touch_y = y;
    is_pressed = press;
}

void get_coor(uint16_t* x, uint16_t* y, bool* press)
{
    *x = touch_x;
    *y = touch_y;
    *press = is_pressed;
}

esp_err_t touch_read(void)
{
    esp_err_t err = ESP_OK;
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;
    err = esp_lcd_touch_read_data(tp);
    if (err != ESP_OK)
    {
        DISPLAY_INFO("GT911 read error");
        return err;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 1)) {
        DISPLAY_INFO("X=%hu Y=%hu strenth=%hu cnt=%d", touch_x[0], touch_y[0], touch_strength[0], touch_cnt);
        set_coor(touch_x[0], touch_y[0], true);
    }
    else {
        set_coor(0xffff, 0xffff, false);
    }
    return err;
}

#endif

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
    if (err != ESP_OK)
        return err;
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
    if (err != ESP_OK)
        return err;
    err = ledc_channel_config(&channel_config);
    if (err != ESP_OK)
        return err;
    return err;
}

esp_err_t set_lcd_blight(uint32_t brightness)
{
    esp_err_t err = ESP_OK;
    if (brightness != 0)
    {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, ((brightness * 18) + 200));
        if (err != ESP_OK)
            return err;
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        if (err != ESP_OK)
            return err;
    }
    else
    {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        if (err != ESP_OK)
            return err;
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        if (err != ESP_OK)
            return err;
    }
    return err;
}

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
    if (err != ESP_OK)
        return err;

    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    err = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io);
    if (err != ESP_OK)
        return err;

    if (BITS_PER_PIXEL == 24)
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB888;
    else if (BITS_PER_PIXEL == 18)
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB666;
    else if (BITS_PER_PIXEL == 16)
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    const esp_lcd_dpi_panel_config_t dpi_config = {
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 51,
        .virtual_channel = 0,
        .pixel_format = dpi_pixel_format,
        .num_fbs = 2,
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
    if (err != ESP_OK)
        return err;
    err = esp_lcd_panel_reset(panel_handle);
    if (err != ESP_OK)
        return err;
    err = esp_lcd_panel_init(panel_handle);
    if (err != ESP_OK)
        return err;
    return err;
}

static esp_err_t lvgl_init()
{
    esp_err_t err = ESP_OK;
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = configMAX_PRIORITIES - 4, /* LVGL task priority */
        .task_stack = 8192*2,                        /* LVGL task stack size */
        .task_affinity = -1,                       /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 10,                   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5,                      /* LVGL timer tick period in ms */
    };
    err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK)
    {
        DISPLAY_ERROR("LVGL port initialization failed");
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = mipi_dbi_io,
        .panel_handle = panel_handle,
        .control_handle = panel_handle,
        .buffer_size = H_size * V_size,
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
            .direct_mode = true,
#endif
        },
    };
    const lvgl_port_display_dsi_cfg_t lvgl_dpi_cfg = {
        .flags = {
#if CONFIG_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = true,
#endif
        },
    };
    my_lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &lvgl_dpi_cfg);
    if (my_lvgl_disp == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL dsi port add fail");
    }
#ifdef CONFIG_BSP_TOUCH_ENABLED
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = my_lvgl_disp,
        .handle = tp,
    };
    my_touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (my_touch_indev == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL touch port add fail");
    }
#endif
    return err;
}

void fill_screen_with_color(lv_color_t color)
{
    lv_canvas_fill_bg(color_obj, color, LV_OPA_COVER);
}

void set_canvas_display(bool state)
{
    if (state)
    {
        if (color_obj != NULL)
            lv_obj_clear_flag(color_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (color_obj != NULL)
            lv_obj_add_flag(color_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

esp_err_t display_init()
{
    esp_err_t err = ESP_OK;
    err = blight_init();
    if (err != ESP_OK)
        return err;
    err = display_port_init();
    if (err != ESP_OK)
        return err;
    err = lvgl_init();
    if (err != ESP_OK) {
        DISPLAY_ERROR("Display init fail");
        return err;
    }
    if (lvgl_port_lock(0)) {
        set_lcd_blight(0);
        size_t color_buffer_size = 0;
        color_buffer_size = 1024 * 600 * ((BITS_PER_PIXEL + 7) / 8);
        color_buffer = heap_caps_malloc(color_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        color_obj = lv_canvas_create(lv_scr_act());
        lv_obj_set_size(color_obj, 1024, 600);
        lv_canvas_set_buffer(color_obj, color_buffer, 1024, 600, LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(color_obj, LV_ALIGN_CENTER, 0, 0);
        fill_screen_with_color(LV_COLOR_BLACK);

        lvgl_port_unlock();
    }
    return ESP_OK;
}

esp_err_t get_display_buff(void **buff)
{
    esp_err_t err = ESP_OK;
    err = esp_lcd_dpi_panel_get_frame_buffer(panel_handle, 1, buff);
    return err;
}

#endif
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/