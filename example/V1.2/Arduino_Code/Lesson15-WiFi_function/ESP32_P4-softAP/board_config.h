/**
 * @file board_config.h
 * @brief Defines the hardware and library configuration used by the lesson.
 *
 * This file belongs to the Lesson15-WiFi_function course project for the
 * 10.1-inch CrowPanel Advanced (ESP32-P4) board. Comments explain the teaching
 * flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Board Config Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

#pragma once

/**
 * Hardware revision of the 10.1-inch CrowPanel Advanced board.
 * The two revisions differ only in the ESP32-C6 reset line:
 *   V1.1 -> ESP32-C6 reset on IO32, wireless-module socket on IO53/IO54
 *   V1.2 -> ESP32-C6 reset on IO54, wireless-module socket moved to IO27/IO28
 * Keep the line that matches the board in your hand and comment out the other.
 */
#define FIRMWARE_VERSION_V1_1
// #define FIRMWARE_VERSION_V1_2

/*********************** Pin define ***********************/
/**
 * SDIO Interface Pins for ESP-Hosted-MCU
 * Used for high-speed communication between ESP32-P4 (Host) and ESP32-C6 (Slave)
 */
#if defined(FIRMWARE_VERSION_V1_1)

#define WIFI_HOSTED_SDIO_PIN_CLK            (18) // SDIO Serial Clock
#define WIFI_HOSTED_SDIO_PIN_CMD            (19) // SDIO Command/Response line
#define WIFI_HOSTED_SDIO_PIN_D0             (17) // SDIO Data line 0
#define WIFI_HOSTED_SDIO_PIN_D1             (16) // SDIO Data line 1
#define WIFI_HOSTED_SDIO_PIN_D2             (15) // SDIO Data line 2
#define WIFI_HOSTED_SDIO_PIN_D3             (14) // SDIO Data line 3 (4-bit mode)
#define WIFI_HOSTED_SDIO_PIN_RESET          (32) // Hardware Reset for the ESP32-C6 co-processor

#elif defined(FIRMWARE_VERSION_V1_2)

#define WIFI_HOSTED_SDIO_PIN_CLK            (18) // SDIO Serial Clock
#define WIFI_HOSTED_SDIO_PIN_CMD            (19) // SDIO Command/Response line
#define WIFI_HOSTED_SDIO_PIN_D0             (17) // SDIO Data line 0
#define WIFI_HOSTED_SDIO_PIN_D1             (16) // SDIO Data line 1
#define WIFI_HOSTED_SDIO_PIN_D2             (15) // SDIO Data line 2
#define WIFI_HOSTED_SDIO_PIN_D3             (14) // SDIO Data line 3 (4-bit mode)
#define WIFI_HOSTED_SDIO_PIN_RESET          (54) // Hardware Reset for the ESP32-C6 co-processor

#endif
/*********************** Pin define ***********************/
