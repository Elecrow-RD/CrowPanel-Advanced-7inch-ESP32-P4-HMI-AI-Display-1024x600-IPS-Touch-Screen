/**
 * @file ESP32_P4-softAP.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson15-WiFi_function course project for the
 * 10.1-inch CrowPanel Advanced (ESP32-P4) board. Comments explain the teaching
 * flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Esp32 P4-Soft Ap Module
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
const char* ap_ssid      = "ELECROW";
const char* ap_password  = "12345678";
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

    // Initialize WiFi in Access Point mode
    // Parameters: SSID, Password, Channel, Hidden(bool), Max_Connections
    if (WiFi.softAP(ap_ssid, ap_password)) {
        Serial.println("Access Point Started Successfully");
    } else {
        Serial.println("Access Point Failed to Start");
    }

    // Default IP for AP mode is usually 192.168.4.1
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
}

/**
 * @brief Run the lesson's recurring application work.
 *
 * Called repeatedly by the Arduino runtime after setup completes.
 *
 * @return Nothing.
 */
void loop() {
    // Monitor connected clients
    int clients = WiFi.softAPgetStationNum();
    Serial.printf("Connected clients: %d\n", clients);
    
    delay(2000);
}
