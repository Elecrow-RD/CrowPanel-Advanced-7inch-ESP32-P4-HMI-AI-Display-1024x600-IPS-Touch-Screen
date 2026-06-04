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

#include "esp_panel_drivers_conf.h"
#include "esp_panel_board_custom_conf.h"
#include "ESP_Panel_Library.h"

/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ORIGINAL(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__);
#define PRINTF_PRINT(fmt, ...)    Serial.print(fmt);
#define PRINTF_LN(fmt, ...)       Serial.println(fmt);

#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("ERROR"); \
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

// Touch panel resolution
#define V_size 600      // Vertical resolution (Y-axis)
#define H_size 1024     // Horizontal resolution (X-axis)
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
static const char *TAG = "TOUCH_APP";  // Tag for logging messages

// --- Declare the panel pointer globally ---
ESP_Panel *panel = nullptr;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

void setup() {
  // put your setup code here, to run once:

  // Initialize the default Serial for debugging (UART0)
  Serial.begin(115200);

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

  // --- Initialize Display and Touch Panel ---
  panel = new ESP_Panel();

  // Initialize the bus (MIPI-DSI) and the devices (EK79007 & GT911)
  // The library uses settings from ESP_Panel_Conf.h internally
  Serial.println("Initializing Panel (EK79007 + GT911)...");
  if (!panel->init()) {
    Serial.println("Panel Initialization Failed!");
    while (1) delay(100);
  }

  // Begin the panel (Startup sequences and backlight)
  if (!panel->begin()) {
    Serial.println("Panel Start Failed!");
    while (1) delay(100);
  }

  Serial.println("Display and Touch system online.");
}

void loop() {
  // put your main code here, to run repeatedly:

  // --- Process Touch Inputs ---
  auto touch = panel->getTouch();
  
  if (touch != nullptr) {
    // --- Read data from the hardware ---
    // Use the correct struct name: ESP_PanelTouchPoint
    ESP_PanelTouchPoint point[10];  // save 10 points

    // --- Check the number of touch points ---
    int point_num = touch->readPoints(point, 10, 0);  // readPoints() ==> readRawData() + getPoints()

    for (int i=0; i<point_num; i++) {
        // Print the coordinates to Serial
        Serial.printf("Touch point[%d]: x %4d, y %3d, strength %d\n", i, point[i].x, point[i].y, point[i].strength);
    }

    if (!touch->isInterruptEnabled()) {
        delay(20);  // Small delay to maintain stability
    }
  } else {
    Serial.println("IDLE loop");
    delay(1000);
  }
}
