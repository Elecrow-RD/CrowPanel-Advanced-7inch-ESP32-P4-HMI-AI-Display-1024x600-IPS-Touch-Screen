 #include "wifi_board.h"
#include "codecs/no_audio_codec.h"  // Use NoAudioCodec (direct I2S connection, no external codec)
#include "application.h"
#include "display/lcd_display.h"
#include "button.h"
#include "config.h"
#include <ssid_manager.h>

// MIPI DSI display related headers
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_ek79007.h"  // EK79007 display driver chip

#include <wifi_station.h>
#include <ssid_manager.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lvgl_port.h>
#include "esp_lcd_touch_gt911.h"  // If touch screen is supported
#include "p4_bsp_camera.h"

#define TAG "ElecrowP4Board"

// If your backlight needs special control, you can implement this class
// If not needed, you can delete this class
class CustomBacklight : public Backlight {
public:
    CustomBacklight(i2c_master_bus_handle_t i2c_handle)
        : Backlight(), i2c_handle_(i2c_handle) {}

protected:
    i2c_master_bus_handle_t i2c_handle_;

    virtual void SetBrightnessImpl(uint8_t brightness) override {
        // If your backlight is controlled via I2C, implement it here
        // If not needed, leave empty or delete this class
        ESP_LOGI(TAG, "Set backlight brightness to %u", brightness);
    }
};

class ElecrowP4Board : public WifiBoard {
private:
    i2c_master_bus_handle_t touch_i2c_bus_;  // Touch screen I2C bus
    Button boot_button_;
    LcdDisplay *display_;
    Backlight *backlight_;  // If backlight control is not needed, can be set to nullptr
    P4BspCamera *camera_;  // P4 BSP camera

    // Initialize PA control pin (power amplifier enable)
    void InitializeAudioCtrl() {
        gpio_config_t audio_gpio_config = {
            .pin_bit_mask = 1ULL << AUDIO_CODEC_PA_PIN,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&audio_gpio_config));
        ESP_LOGI(TAG, "Audio PA control GPIO initialized");
    }

    // Enable MIPI DSI PHY power (LDO3: 2.5V)
    static esp_err_t bsp_enable_dsi_phy_power(void) {
#if MIPI_DSI_PHY_PWR_LDO_CHAN > 0
        static esp_ldo_channel_handle_t phy_pwr_chan = NULL;
        esp_ldo_channel_config_t ldo_cfg = {
            .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN,
            .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV,
        };
        esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr_chan);
        ESP_LOGI(TAG, "MIPI DSI PHY Powered on (LDO%d: %dmV)", MIPI_DSI_PHY_PWR_LDO_CHAN, MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV);
#endif
        return ESP_OK;
    }

    // Initialize camera power (LDO4: 3.3V, based on the provided main.c)
    static esp_err_t bsp_enable_camera_power(void) {
        static esp_ldo_channel_handle_t camera_pwr_chan = NULL;
        esp_ldo_channel_config_t ldo_cfg = {
            .chan_id = 4,  // LDO4
            .voltage_mv = 3300,  // 3.3V
        };
        esp_err_t err = esp_ldo_acquire_channel(&ldo_cfg, &camera_pwr_chan);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable camera power (LDO4): %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "Camera power enabled (LDO4: 3300mV)");
        return ESP_OK;
    }

    // Initialize display (EK79007 + MIPI DSI)
    void InitializeLCD() {
        bsp_enable_dsi_phy_power();

        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;
        esp_lcd_dsi_bus_handle_t mipi_dsi_bus = nullptr;

        // Create MIPI DSI bus (refer to bsp_illuminate.c)
        esp_lcd_dsi_bus_config_t bus_config = {
            .bus_id = 0,
            .num_data_lanes = LCD_MIPI_DSI_LANE_NUM,
            .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
            .lane_bit_rate_mbps = 900,
        };
        ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

        // Create DBI interface (for sending LCD commands)
        esp_lcd_dbi_io_config_t dbi_config = {
            .virtual_channel = 0,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &panel_io));

        // Configure DPI parameters (resolution 1024x600, timing refers to bsp_illuminate.c)
        lcd_color_rgb_pixel_format_t pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
        esp_lcd_dpi_panel_config_t dpi_config = {
            .virtual_channel = 0,
            .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
            .dpi_clock_freq_mhz = 51,
            .pixel_format = pixel_format,
            .num_fbs = 1,
            .video_timing = {
                .h_size = DISPLAY_WIDTH,
                .v_size = DISPLAY_HEIGHT,
                .hsync_pulse_width = 70,
                .hsync_back_porch = 160,
                .hsync_front_porch = 160,
                .vsync_pulse_width = 10,
                .vsync_back_porch = 23,
                .vsync_front_porch = 12,
            },
            .flags = {
                .use_dma2d = true,
            },
        };

        // EK79007 specific configuration
        ek79007_vendor_config_t vendor_config = {
            .mipi_config = {
                .dsi_bus = mipi_dsi_bus,
                .dpi_config = &dpi_config,
            },
        };

        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = PIN_NUM_LCD_RST,          // GPIO_NUM_NC if no reset pin
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = LCD_BIT_PER_PIXEL,
            .vendor_config = &vendor_config,
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_ek79007(panel_io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

        // Create display object (LVGL integration handled by MipiLcdDisplay)
        display_ = new MipiLcdDisplay(panel_io, panel,
                                      DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                      DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                      DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
                                      DISPLAY_SWAP_XY);

        // Backlight uses generic PwmBacklight (can be further adapted if needed)
        backlight_ = nullptr;

        ESP_LOGI(TAG, "LCD (EK79007) initialized: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    // Initialize touch screen I2C bus
    void InitializeTouchI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = TOUCH_I2C_PORT,
            .sda_io_num = TOUCH_I2C_SDA_PIN,
            .scl_io_num = TOUCH_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &touch_i2c_bus_));
        ESP_LOGI(TAG, "Touch I2C bus initialized: SDA=%d, SCL=%d", TOUCH_I2C_SDA_PIN, TOUCH_I2C_SCL_PIN);
    }

    // Initialize touch screen (GT911)
    void InitializeTouch() {
        // Skip if I2C is not initialized
        if (touch_i2c_bus_ == nullptr) {
            ESP_LOGW(TAG, "Touch I2C bus not initialized, skipping touch panel");
            return;
        }
        
        esp_lcd_touch_handle_t tp;
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = DISPLAY_WIDTH,
            .y_max = DISPLAY_HEIGHT,
            .rst_gpio_num = TOUCH_GPIO_RST,
            .int_gpio_num = TOUCH_GPIO_INT,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        };
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        tp_io_config.scl_speed_hz = 400 * 1000;  // 400kHz I2C speed (based on your bsp_display.c configuration)
        // Try primary address
        esp_err_t err = esp_lcd_new_panel_io_i2c(touch_i2c_bus_, &tp_io_config, &tp_io_handle);
        if (err != ESP_OK) {
            // If primary address fails, try backup address
            tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
            ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(touch_i2c_bus_, &tp_io_config, &tp_io_handle));
        }
        ESP_LOGI(TAG, "Initialize touch controller GT911");
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lv_display_get_default(),
            .handle = tp,
        };
        lvgl_port_add_touch(&touch_cfg);
        ESP_LOGI(TAG, "Touch panel initialized successfully: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    // Initialize buttons
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && 
                !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
        ESP_LOGI(TAG, "Buttons initialized");
    }

    // Initialize camera
    void InitializeCamera() {
        try {
            camera_ = new P4BspCamera();
            ESP_LOGI(TAG, "Camera initialized successfully");
        } catch (...) {
            ESP_LOGE(TAG, "Failed to initialize camera");
            camera_ = nullptr;
        }
    }

public:
    // Constructor: initialize all hardware
    ElecrowP4Board() : touch_i2c_bus_(nullptr), boot_button_(BOOT_BUTTON_GPIO), camera_(nullptr) {
        ESP_LOGI(TAG, "Initializing Elecrow P4 Board...");
        
        // Initialize GPIO ISR service (camera may need this)
        esp_err_t err = gpio_install_isr_service(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(err));
        }
        
        InitializeAudioCtrl();
        InitializeLCD();
        //InitializeTouchI2c();  // Initialize touch screen I2C bus
        //InitializeTouch();     // Initialize touch screen (if I2C is configured)
        InitializeButtons();
        // Enable backlight (important! otherwise screen will be black)
        GetBacklight()->RestoreBrightness();
        // Initialize camera power (LDO4)
        bsp_enable_camera_power();
        // Initialize camera
        InitializeCamera();
        ESP_LOGI(TAG, "Elecrow P4 Board initialized successfully");
    }

    // Get audio codec
    // Use NoAudioCodecSimplexPdm: speaker uses I2S standard mode, microphone uses PDM mode
    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplexPdm audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,   // Microphone sample rate: 16000
            AUDIO_OUTPUT_SAMPLE_RATE,  // Speaker sample rate: 16000
            AUDIO_I2S_GPIO_BCLK,       // Speaker BCLK pin
            AUDIO_I2S_GPIO_WS,         // Speaker WS/LRCLK pin
            AUDIO_I2S_GPIO_DOUT,       // Speaker DOUT pin
            AUDIO_PDM_MIC_CLK,         // PDM microphone clock pin
            AUDIO_PDM_MIC_DIN);        // PDM microphone data input pin
        return &audio_codec;
    }

    // Get display
    virtual Display* GetDisplay() override {
        return display_;
    }

    // Get backlight controller
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT, 30000);
        return &backlight;
    }

    // Get camera
    virtual Camera* GetCamera() override {
        return camera_;
    }

    // Override StartNetwork method to automatically add default Wi-Fi configuration
    virtual void StartNetwork() override {
        ESP_LOGI(TAG, "ElecrowP4Board::StartNetwork() called");
        
        // In ESP-Hosted mode, wait for C6 coprocessor to be ready
        // Give SDIO communication enough time to establish connection
        ESP_LOGI(TAG, "Waiting for ESP32-C6 coprocessor to be ready...");
        vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds for C6 to start and initialize SDIO
        
        // Check if Wi-Fi configuration already exists
        auto& ssid_manager = SsidManager::GetInstance();
        auto ssid_list = ssid_manager.GetSsidList();
        
        ESP_LOGI(TAG, "Current WiFi SSID list size: %d", ssid_list.size());
        for (size_t i = 0; i < ssid_list.size(); i++) {
            ESP_LOGI(TAG, "  [%d] SSID: %s", i, ssid_list[i].ssid.c_str());
        }
        
        // If not configured, add default Wi-Fi
        if (ssid_list.empty()) {
            ESP_LOGI(TAG, "No WiFi configured, adding default WiFi: elecrow888");
            ssid_manager.AddSsid("elecrow888", "elecrow2014");
            ESP_LOGI(TAG, "Default WiFi added successfully");
        } else {
            // Check if elecrow888 already exists, if not then add it
            bool found = false;
            for (const auto& item : ssid_list) {
                if (item.ssid == "elecrow888") {
                    found = true;
                    ESP_LOGI(TAG, "WiFi 'elecrow888' already configured");
                    break;
                }
            }
            if (!found) {
                ESP_LOGI(TAG, "Adding default WiFi: elecrow888");
                ssid_manager.AddSsid("elecrow888", "elecrow2014");
                ESP_LOGI(TAG, "Default WiFi added successfully");
            }
        }
        
        // Call parent class method for normal network startup process
        WifiBoard::StartNetwork();
    }
};

// Register board (required!)
DECLARE_BOARD(ElecrowP4Board);
