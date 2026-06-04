#pragma once

/*********************** Pin define ***********************/

// GPIO pins for GT911 touch panel
#define Touch_GPIO_RST      (40)    // Reset pin
#define Touch_GPIO_INT      (42)    // Interrupt pin

// GPIO pins for I2C, has touch chip GT911
#define I2C_GPIO_SCL        (46)    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA        (45)    // GPIO number used for I2C SDA (data) line

// GPIO pins for display backlight
#define LCD_GPIO_BLIGHT     (31)    // LCD Blight GPIO
#define BLIGHT_PWM_Hz       (30000) // LCD Blight PWM Hz
#define BLIGHT_ON_LEVEL     (1)     // Active level, 0: low, 1: high

// GPIO pins for display
#define LCD_GPIO_RST        (41)    // LCD Blight GPIO

// panel parameters
#define V_size              (600)  // Vertical resolution (Y-axis)
#define H_size              (1024)  // Horizontal resolution (X-axis)
#define LCD_CLK_MHZ         (51)
#define LCD_HPW             (70)
#define LCD_HBP             (160)
#define LCD_HFP             (160)
#define LCD_VPW             (10)
#define LCD_VBP             (23)
#define LCD_VFP             (21)
/*********************** Pin define ***********************/
