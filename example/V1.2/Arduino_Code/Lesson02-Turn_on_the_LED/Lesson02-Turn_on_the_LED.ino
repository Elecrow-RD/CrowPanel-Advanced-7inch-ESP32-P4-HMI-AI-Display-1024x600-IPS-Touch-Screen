/**
 * @file Lesson02-Turn_on_the_LED.ino
 * @brief Teaching example that demonstrates how to blink the onboard LED at a one-second interval.
 *
 * Comments emphasize program structure, hardware intent, and call order
 * while preserving the original executable behavior.
 */

#include "config.h"
/*---------------------------------------------------------------
 * Arduino entry points
 * The Arduino runtime calls setup() once and loop() repeatedly.
 *--------------------------------------------------------------*/
/**
 * @brief Initialize the hardware and services needed to blink the onboard LED at a one-second interval.
 *
 * Parameters: None.
 * @return None.
 * @note Called once by the Arduino runtime after reset.
 */
void setup() {

  // Initialize GPIO
  pinMode(PIN_LED, OUTPUT);
    
}

/**
 * @brief Continue the runtime workflow used to blink the onboard LED at a one-second interval.
 *
 * Parameters: None.
 * @return None.
 * @note Called repeatedly by the Arduino runtime after setup() returns.
 */
void loop() {

  // Holding each level for one second makes the LED state change easy to observe.
  digitalWrite(PIN_LED, HIGH);
  delay(1000);


  digitalWrite(PIN_LED, LOW);
  delay(1000);
}
