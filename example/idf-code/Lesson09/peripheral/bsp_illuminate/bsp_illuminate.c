/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_illuminate.h"  // Include BSP display header (LCD and related configurations)
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

esp_lcd_panel_handle_t panel_handle = NULL;          /* Type of LCD panel handle */
static esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL; /* Type of MIPI DSI bus handle */
static esp_lcd_panel_io_handle_t mipi_dbi_io = NULL; /* Type of LCD panel IO handle */
static lv_display_t *my_lvgl_disp = NULL;            /* Backward compatibility with LVGL 8 */
static lv_indev_t *my_touch_indev;

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

static esp_err_t blight_init(void)  // Initialize LCD backlight (PWM control)
{
    esp_err_t err = ESP_OK;  // Error status variable
    const gpio_config_t gpio_cofig = {  // GPIO configuration for backlight pin
        .pin_bit_mask = (1ULL << LCD_GPIO_BLIGHT),  // Select backlight GPIO pin
        .mode = GPIO_MODE_OUTPUT,                   // Configure as output
        .pull_up_en = false,                        // Disable pull-up
        .pull_down_en = false,                      // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE,             // Disable interrupt
    };
    err = gpio_config(&gpio_cofig);  // Apply GPIO config
    if (err != ESP_OK)
        return err;  // Return error if failed

    const ledc_timer_config_t timer_config = {  // LEDC timer configuration
        .clk_cfg = LEDC_USE_PLL_DIV_CLK,        // Use PLL clock
        .duty_resolution = LEDC_TIMER_11_BIT,   // 11-bit duty resolution
        .freq_hz = BLIGHT_PWM_Hz,               // Backlight PWM frequency
        .speed_mode = LEDC_LOW_SPEED_MODE,      // Low-speed mode
        .timer_num = LEDC_TIMER_0,              // Timer 0
    };

    const ledc_channel_config_t channel_config = {  // LEDC channel configuration
        .gpio_num = LCD_GPIO_BLIGHT,                // Backlight GPIO pin
        .speed_mode = LEDC_LOW_SPEED_MODE,          // Low-speed mode
        .channel = LEDC_CHANNEL_0,                  // Channel 0
        .intr_type = LEDC_INTR_DISABLE,             // Disable interrupt
        .timer_sel = LEDC_TIMER_0,                  // Use timer 0
        .duty = 0,                                  // Initial duty = 0
        .hpoint = 0,                                // Start point
    };
    err = ledc_timer_config(&timer_config);  // Configure LEDC timer
    if (err != ESP_OK)
        return err;
    err = ledc_channel_config(&channel_config);  // Configure LEDC channel
    if (err != ESP_OK)
        return err;     
    return err;  // Return success
}

/* brightness -  (0 - 100) */
esp_err_t set_lcd_blight(uint32_t brightness)  // Set LCD backlight brightness
{
    esp_err_t err = ESP_OK;
    if (brightness != 0)  // If brightness > 0
    {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, ((brightness * 18) + 200));  // Calculate PWM duty
        if (err != ESP_OK)
            return err;
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);  // Apply duty update
        if (err != ESP_OK)
            return err;
    }
    else  // If brightness = 0, turn off
    {
        err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);  // Set duty = 0
        if (err != ESP_OK)
            return err;
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);  // Apply duty update
        if (err != ESP_OK)
            return err;
    }
    return err;  // Return status
}

static esp_err_t display_port_init(void)  // Initialize LCD port (MIPI DSI + panel config)
{
    esp_err_t err = ESP_OK;
    lcd_color_rgb_pixel_format_t dpi_pixel_format;  // Pixel format variable
    esp_lcd_dsi_bus_config_t bus_config = {  // MIPI DSI bus config
        .bus_id = 0,                        // Bus ID = 0
        .num_data_lanes = 2,                // 2 data lanes
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,  // Default PHY clock source
        .lane_bit_rate_mbps = 900,          // Lane bit rate = 900 Mbps
    };
    err = esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);  // Create new DSI bus
    if (err != ESP_OK)
        return err;
    esp_lcd_dbi_io_config_t dbi_config = {  // DBI interface config
        .virtual_channel = 0,               // Virtual channel = 0
        .lcd_cmd_bits = 8,                  // 8-bit command
        .lcd_param_bits = 8,                // 8-bit parameter
    };
    err = esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io);  // Create new DBI IO
    if (err != ESP_OK)
        return err; 
    if (BITS_PER_PIXEL == 24)
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB888;  // 24-bit RGB888
    else if (BITS_PER_PIXEL == 18)
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB666;  // 18-bit RGB666
    else if (BITS_PER_PIXEL == 16)
        dpi_pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;  // 16-bit RGB565

    const esp_lcd_dpi_panel_config_t dpi_config = {  // DPI panel config
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,  // Default DPI clock source
        .dpi_clock_freq_mhz = 51,                     // DPI clock frequency = 51 MHz
        .virtual_channel = 0,                         // Virtual channel = 0
        .pixel_format = dpi_pixel_format,             // Pixel format
        .num_fbs = 1,                                 // Frame buffers = 1
        .video_timing = {                             // Video timing parameters
            .h_size = H_size,                         // Horizontal size
            .v_size = V_size,                         // Vertical size
            .hsync_back_porch = 160,                  // HSync back porch
            .hsync_pulse_width = 70,                  // HSync pulse width
            .hsync_front_porch = 160,                 // HSync front porch
            .vsync_back_porch = 23,                   // VSync back porch
            .vsync_pulse_width = 10,                  // VSync pulse width
            .vsync_front_porch = 12,                  // VSync front porch
        },
        .flags.use_dma2d = true,                      // Enable DMA2D
    };

    ek79007_vendor_config_t vendor_config = {  // Vendor-specific config
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,        // DSI bus handle
            .dpi_config = &dpi_config,      // DPI config reference
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {  // Panel device config
        .reset_gpio_num = -1,                // No reset GPIO
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // RGB order
        .bits_per_pixel = BITS_PER_PIXEL,    // Pixel depth
        .vendor_config = &vendor_config,     // Vendor config pointer
    };
    err = esp_lcd_new_panel_ek79007(mipi_dbi_io, &panel_config, &panel_handle);  // Create new EK79007 panel
    if (err != ESP_OK)
        return err;
    err = esp_lcd_panel_reset(panel_handle);  // Reset panel
    if (err != ESP_OK)
        return err;
    err = esp_lcd_panel_init(panel_handle);   // Initialize panel
    if (err != ESP_OK)
        return err;
    return err;  // Return success
}

static esp_err_t lvgl_init()  // Initialize LVGL
{
    
    esp_err_t err = ESP_OK;
    const lvgl_port_cfg_t lvgl_cfg = {  // LVGL port configuration
        .task_priority = configMAX_PRIORITIES - 4, /* LVGL task priority */
        .task_stack = 8192*2,                      /* LVGL task stack size */
        .task_affinity = -1,                       /* Task pinned to core (-1 = no affinity) */
        .task_max_sleep_ms = 10,                   /* Max sleep in LVGL task */
        .timer_period_ms = 5,                      /* LVGL timer tick period in ms */
    };
    err = lvgl_port_init(&lvgl_cfg);  // Initialize LVGL port
    if (err != ESP_OK)
    {
        ILLUMINATE_ERROR("LVGL port initialization failed");
    }

    const lvgl_port_display_cfg_t disp_cfg = {  // LVGL display configuration
        .io_handle = mipi_dbi_io,                // IO handle
        .panel_handle = panel_handle,            // Panel handle
        .control_handle = panel_handle,          // Control handle
        .buffer_size = (H_size * V_size * ((BITS_PER_PIXEL + 7) / 8)),  // Frame buffer size
        .double_buffer = true,                   // Enable double buffer
        .hres = H_size,                          // Horizontal resolution
        .vres = V_size,                          // Vertical resolution
        .monochrome = false,                     // Not monochrome
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,  // Color format
#endif
        .rotation = {                            // Rotation config
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {                               // Display flags
            .buff_dma = false,                   // Buffer not in DMA
            .buff_spiram = true,                 // Buffer in SPIRAM
            .sw_rotate = true,                   // Enable software rotation
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,                  // Swap bytes (LVGL v9+)
#endif
#if CONFIG_DISPLAY_LVGL_FULL_REFRESH
            .full_refresh = true,                // Enable full refresh
#else
            .full_refresh = false,               // Disable full refresh
#endif
#if CONFIG_DISPLAY_LVGL_DIRECT_MODE
            .direct_mode = true,                 // Enable direct mode
#else
            .direct_mode = false,                // Disable direct mode
#endif
        },
    };
    const lvgl_port_display_dsi_cfg_t lvgl_dpi_cfg = {  // LVGL DSI configuration
        .flags = {
#if CONFIG_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,               // Enable tearing avoidance
#else
            .avoid_tearing = false,              // Disable tearing avoidance
#endif
        },
    };
    my_lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &lvgl_dpi_cfg);  // Add LVGL display
    if (my_lvgl_disp == NULL)
    {
        err = ESP_FAIL;
        ILLUMINATE_ERROR("LVGL dsi port add fail");
    }
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
    return err;
}

esp_err_t display_init()  // Display initialization function
{
    esp_err_t err = ESP_OK;
    err = blight_init(); /* Backlight initialization function */
    if (err != ESP_OK)
        return err;
    err = display_port_init(); /* Display screen interface initialization function */
    if (err != ESP_OK)
        return err;
    err = lvgl_init(); /* Screen interface registration LVGL function */
    if (err != ESP_OK)
    {
        ILLUMINATE_ERROR("Display init fail");
        return err;
    }
    set_lcd_blight(0);  /* Set backlight to off */
    return err;
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
