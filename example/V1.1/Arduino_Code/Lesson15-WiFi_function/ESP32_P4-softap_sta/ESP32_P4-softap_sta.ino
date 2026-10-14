/**
 * @file ESP32_P4-softap_sta.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson15-WiFi_function course project for the
 * 10.1-inch CrowPanel Advanced (ESP32-P4) board. Comments explain the teaching
 * flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Esp32 P4-Softap Sta Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"           // board pin define

/* Wireless Connectivity (WiFi) */
#include <WiFi.h>                   // Standard ESP32 WiFi library
#include <lwip/lwip_napt.h>         // Required for NAT functionality
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
const char* ap_ssid      = "ELECROW";
const char* ap_password  = "12345678";

const char* sta_ssid     = "yanfa_software";
const char* sta_password = "yanfa-123456";
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief Initialize the hardware and services required by this lesson.
 *
 * Called once by the Arduino runtime after power-up or reset.
 *
 * @return Nothing.
 */
void setup() {
    Serial.begin(115200);

    // ESP-Hosted-MCU SDIO Interface Pins for WiFi
    // 10.1-inch CrowPanel Advanced: CLK=18, CMD=19, D0..D3=17/16/15/14, RESET=32 (V1.1 board)
    WiFi.setPins(
        WIFI_HOSTED_SDIO_PIN_CLK, 
        WIFI_HOSTED_SDIO_PIN_CMD, 
        WIFI_HOSTED_SDIO_PIN_D0, 
        WIFI_HOSTED_SDIO_PIN_D1,
        WIFI_HOSTED_SDIO_PIN_D2, 
        WIFI_HOSTED_SDIO_PIN_D3, 
        WIFI_HOSTED_SDIO_PIN_RESET
    );

    // 1. Set mode to AP + STA
    WiFi.mode(WIFI_AP_STA);

    // 2. Start AP
    WiFi.softAP(ap_ssid, ap_password);
    
    // 3. Connect to Router
    WiFi.begin(sta_ssid, sta_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // 4. ENABLE NAT (The magic happens here)
    // This allows traffic to flow from AP to STA
    ip_napt_enable(WiFi.softAPIP(), 1); 
    
    Serial.println("\nNAT Enabled. Mobile device should have internet now.");
}

/**
 * @brief Run the lesson's recurring application work.
 *
 * Called repeatedly by the Arduino runtime after setup completes.
 *
 * @return Nothing.
 */
void loop() {
    // Check Station connection status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[STA] Connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("[STA] Connecting to router...");
    }
    
    // Display AP status
    Serial.print("[AP] Hotspot IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("[AP] Stations connected: ");
    Serial.println(WiFi.softAPgetStationNum());

    delay(5000);
}
