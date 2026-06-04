/**
 * IMPORTANT:
 * This code requires the "ESP32_Display_Panel" library.
 * Before uploading, you MUST configure the following file in your libraries folder:
 * [Arduino_Library_Path]/ESP32_Display_Panel/esp_panel_drivers_conf.h
 * 
 * 1. Enable MIPI-DSI: #define ESP_PANEL_DRIVERS_BUS_USE_MIPI_DSI   (1)
 * 2. Set LCD Driver:  #define ESP_PANEL_DRIVERS_LCD_USE_EK79007    (1)
 * 3. Set Touch:       #define ESP_PANEL_DRIVERS_TOUCH_USE_GT911    (1)
 */
/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs

#include <string.h>         // C string lib
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes
#include <esp_ldo_regulator.h>      // ESP32-P4 specific LDO management

/* panel driver */
#include "esp_panel_drivers_conf.h"
#include "esp_panel_board_custom_conf.h"
#include "ESP_Panel_Library.h"

/* LVGL and driver */
#include <lvgl.h>
#include "lvgl_v8_port.h"

/* LoRa */
#include "bsp_wireless.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ORIGINAL(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__);
#define PRINTF_PRINT(fmt, ...)    Serial.print(fmt);
#define PRINTF_LN(fmt, ...)       Serial.println(fmt);

#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("ERROR: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_WARN(fmt, ...)       do { \
                                        Serial.print("WARN: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_INFO(fmt, ...)       do { \
                                        Serial.print("INFO: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_DEBUG(fmt, ...)      do { \
                                        Serial.print("DEBUG: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)

#define MAIN_INFO(fmt, ...)         PRINTF_INFO(fmt, ##__VA_ARGS__)   // Info level log macro
#define MAIN_ERROR(fmt, ...)        PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Error level log macro

#define LV_COLOR_RED        lv_color_make(0xFF, 0x00, 0x00) // LVGL Red
#define LV_COLOR_GREEN      lv_color_make(0x00, 0xFF, 0x00) // LVGL Green
#define LV_COLOR_BLUE       lv_color_make(0x00, 0x00, 0xFF) // LVGL Blue
#define LV_COLOR_WHITE      lv_color_make(0xFF, 0xFF, 0xFF) // LVGL White
#define LV_COLOR_BLACK      lv_color_make(0x00, 0x00, 0x00) // LVGL Black
#define LV_COLOR_GRAY       lv_color_make(0x80, 0x80, 0x80) // LVGL gray
#define LV_COLOR_YELLOW     lv_color_make(0xFF, 0xFF, 0x00) // LVGL yellow
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
static lv_obj_t *s_rx_label = NULL;             // LVGL label object to display received data
static uint32_t rx_packet_count = 0;            // Counter for the number of received nRF24L01 packets
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief Callback function triggered when nRF24L01 data is received
 */
static void rx_data_callback(const char* data, size_t len)
{
    rx_packet_count++;  // Increment the received packet count each time data is received
    
    // (Update LVGL screen display)
    if (lvgl_port_lock(0) == true) {  // Acquire LVGL lock before updating the UI to ensure thread safety
        // (Format received data as NRF24_RX_Hello World:i)
        if (s_rx_label != NULL) {
            char rx_text[64];  // Buffer to store formatted text
            snprintf(rx_text, sizeof(rx_text), "NRF24_RX_Hello World:%lu", (unsigned long)rx_packet_count);
            lv_label_set_text(s_rx_label, rx_text);  // Update the text of the RX label
        }
        
        lvgl_port_unlock();  // Release LVGL lock after updating the UI
    }
    
    char rx_display_text[64];  // Local buffer for logging display
    snprintf(rx_display_text, sizeof(rx_display_text), "NRF24_RX_Hello World:%lu", (unsigned long)rx_packet_count);
    MAIN_INFO("NRF24 RX: %s", rx_display_text);  // Log received data info to console
}

/**
 * @brief Initialize the LVGL display interface for nRF24L01 RX
 */
static void lvgl_show_rx_interface_init(void)
{
    if (lvgl_port_lock(0) != true) {  // Try to lock LVGL before creating objects
        MAIN_ERROR("LVGL lock failed");  // Log error if lock acquisition fails
        return;  // Exit the function
    }

    lv_obj_t *screen = lv_scr_act();  // Get the current active LVGL screen object
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);  // Set the screen background color to white
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);      // Set background opacity to fully opaque

    // (Create the title label)
    lv_obj_t *title_label = lv_label_create(screen);  // Create a new LVGL label for the title
    if (title_label != NULL) {
        lv_label_set_text(title_label, "nRF24L01 RX Receiver");  // Set the title text
        static lv_style_t title_style;  // Define a separate style for the title
        lv_style_init(&title_style);  
        lv_style_set_text_font(&title_style, &lv_font_montserrat_42);  // Use large font for title
        lv_style_set_text_color(&title_style, lv_color_black());       // Set text color
        lv_style_set_bg_opa(&title_style, LV_OPA_TRANSP);              // Make background transparent
        lv_obj_add_style(title_label, &title_style, LV_PART_MAIN);     // Apply the style to title label
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);            // Align title to top center with Y offset
    }

    //  (Create label to show received message)
    s_rx_label = lv_label_create(screen);  // Create label object for RX message display
    if (s_rx_label != NULL) {
        lv_label_set_text(s_rx_label, "NRF24_RX_Hello World:0");  // Set initial text content
        static lv_style_t rx_style;  // Create style for RX label
        lv_style_init(&rx_style);
        lv_style_set_text_font(&rx_style, &lv_font_montserrat_42);  // Font for RX text
        lv_style_set_text_color(&rx_style, lv_color_black());       // Black text color
        lv_style_set_bg_opa(&rx_style, LV_OPA_TRANSP);              // Transparent background
        lv_obj_add_style(s_rx_label, &rx_style, LV_PART_MAIN);      // Apply RX style
        lv_obj_align(s_rx_label, LV_ALIGN_CENTER, 0, -40);          // Align label near center
    }

    lvgl_port_unlock();  // Unlock LVGL after all UI components are created
}

/**
 * @brief nRF24L01 receive task that checks incoming packets continuously
 */
static void nrf24_rx_task(void *param)
{
    while (1) {  // Infinite loop for continuous checking
        //  (Check if data has been received)
        // Note: nRF24L01 doesn't have the same data received flag as SX1262
        // We'll use a reasonable maximum packet size for nRF24L01 (32 bytes)
        received_nrf24_pack_radio(32);  // Handle received packet data with maximum size
        vTaskDelay(10 / portTICK_PERIOD_MS); // Check every 10ms to reduce CPU usage
    }
}

void ldo_init()
{
    // --- Power Configuration (LDO3 for MIPI D-PHY) ---
    // ESP32-P4's MIPI D-PHY requires specific voltage to function.
    // LDO3 is typically routed to the MIPI power rail on P4 hardware.
    esp_err_t err = ESP_OK;
    esp_ldo_channel_handle_t ldo3_handle = NULL;
    esp_ldo_channel_config_t ldo3_cfg = {
        .chan_id = 3,           // LDO Channel 3
        .voltage_mv = 2500,     // Set to 2500mV (2.5V)
    };

    Serial.println("Initializing LDO3 to 2.5V...");
    err = esp_ldo_acquire_channel(&ldo3_cfg, &ldo3_handle);
    if (err != ESP_OK) {
        Serial.printf("LDO3 Power Error: %s\n", esp_err_to_name(err));
    } else {
        Serial.println("LDO3 Power enabled successfully.");
    }
    
    // --- Power Configuration (LDO4 for I2C/touch pull up) ---
    esp_ldo_channel_handle_t ldo4_handle = NULL;
    esp_ldo_channel_config_t ldo4_cfg = {
        .chan_id = 4,           // LDO Channel 4
        .voltage_mv = 3300,     // Set to 3300mV (3.3V)
    };

    Serial.println("Initializing LDO4 to 3.3V...");
    err = esp_ldo_acquire_channel(&ldo4_cfg, &ldo4_handle);
    if (err != ESP_OK) {
        Serial.printf("LDO4 Power Error: %s\n", esp_err_to_name(err));
    } else {
        Serial.println("LDO4 Power enabled successfully.");
    }
}

void display_touch_lvgl_init() 
{
    // --- Initialize Display and Touch Panel ---
    Board *board = new Board();
    // Initialize the bus (MIPI-DSI) and the devices (EK79007 & GT911)
    Serial.println("Initializing Panel (EK79007 + GT911)...");
    assert(board->init());
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
    assert(board->begin());
    Serial.println("Display and Touch system online.");

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());
}

void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

    ldo_init();

    display_touch_lvgl_init();

    // nRF24L01 Wireless RX init (nRF24L01 receiver initialization)
    esp_err_t err = nrf24_rx_init();  // Initialize nRF24L01 receiver
    if (err != ESP_OK) {
        MAIN_ERROR("nRF24L01 Wireless Module RX init...");  // Halt if failed
    }
    MAIN_INFO("The nRF24L01 wireless module RX initialization was successful.");  // Log success

    lvgl_show_rx_interface_init();  // Initialize LVGL user interface
    MAIN_INFO("-------- LVGL RX Interface OK ----------");  // Log successful UI init

    //  (Set callback function for received data)
    nrf24_set_rx_callback(rx_data_callback);  // Register nRF24L01 RX callback function
    MAIN_INFO("RX callback registered");  // Log callback registration success

    // (Create nRF24L01 receiving task)
    xTaskCreatePinnedToCore(nrf24_rx_task, "nrf24_rx", 4096, NULL,
                            configMAX_PRIORITIES - 5, NULL, 1);  // Create FreeRTOS task pinned to core 1

    MAIN_INFO("nRF24L01 RX receiver started, waiting for data...");  // Log start message
}

void loop() {
    // put your main code here, to run repeatedly:
    
}
