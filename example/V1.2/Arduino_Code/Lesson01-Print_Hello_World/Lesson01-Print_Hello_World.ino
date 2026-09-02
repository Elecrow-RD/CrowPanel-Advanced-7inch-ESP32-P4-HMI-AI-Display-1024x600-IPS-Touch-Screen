/**
 * @file Lesson01-Print_Hello_World.ino
 * @brief Teaching example that demonstrates how to send a periodic Hello World message to the serial monitor.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

#include <Arduino.h>
#include <esp_arduino_version.h>

#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 8)
#error "This lesson requires ESP32 Arduino Core 3.3.8 or newer"
#endif

/*---------------------------------------------------------------
 * Arduino entry points
 * The Arduino runtime calls setup() once and loop() repeatedly.
 *--------------------------------------------------------------*/
/**
 * @brief Initialize the hardware and services needed to send a periodic Hello World message to the serial monitor.
 *
 * Parameters: None.
 * @return None.
 * @note Called once by the Arduino runtime after reset.
 */
void setup() {
  Serial.begin(115200);
}

/**
 * @brief Continue the runtime workflow used to send a periodic Hello World message to the serial monitor.
 *
 * Parameters: None.
 * @return None.
 * @note Called repeatedly by the Arduino runtime after setup() returns.
 */
void loop() {
  Serial.print("Hello World!\r\n");
  delay(1000);
}
