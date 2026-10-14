/**
 * @file ESP32_P4-station.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson15-WiFi_function course project for the
 * 10.1-inch CrowPanel Advanced (ESP32-P4) board. Comments explain the teaching
 * flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Esp32 P4-Station Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"           // board pin define

/* Wireless Connectivity (WiFi) */
#include <WiFi.h>                   // Standard ESP32 WiFi library
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
const char* sta_ssid     = "yanfa1";
const char* sta_password = "1223334444yanfa";
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
    
    // Set WiFi to Station mode
    WiFi.mode(WIFI_STA);
    
    // Start connection process
    WiFi.begin(sta_ssid, sta_password);
    Serial.print("Connecting to WiFi");

    // Wait until connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Success message
    Serial.println("\nConnected!");
    Serial.print("Local IP Address: ");
    Serial.println(WiFi.localIP()); 
}

/**
 * @brief Run the lesson's recurring application work.
 *
 * Called repeatedly by the Arduino runtime after setup completes.
 *
 * @return Nothing.
 */
void loop() {
    // Check connection status periodically
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost. Reconnecting...");
        WiFi.begin(sta_ssid, sta_password);
    }
    delay(10000);
}
