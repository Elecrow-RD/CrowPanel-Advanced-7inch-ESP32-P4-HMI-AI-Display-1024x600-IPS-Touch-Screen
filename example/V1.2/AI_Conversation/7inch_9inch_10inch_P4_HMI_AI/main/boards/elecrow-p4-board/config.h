#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

/*---------------------------------------------------------------
 * Wi-Fi configuration
 * Change these two macros when the device needs to use another network.
 *--------------------------------------------------------------*/
#define WIFI_SSID       "elecrow888"
#define WIFI_PASSWORD   "elecrow2014"

/*---------------------------------------------------------------
 * Audio configuration
 * The speaker uses standard I2S while the microphone uses PDM.
 * Both paths run at 16 kHz to match the voice-processing pipeline.
 *--------------------------------------------------------------*/
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// I2S audio pins
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC  // Master clock pin (unused, set to NC)
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_21  // Word select pin (LRCLK, left/right channel)
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_22  // Bit clock pin (BCLK)
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_23  // Data output pin (SDATA, speaker)

// PDM microphone pins
#define AUDIO_PDM_MIC_CLK  GPIO_NUM_24  // PDM microphone clock pin (CLK)
#define AUDIO_PDM_MIC_DIN  GPIO_NUM_26  // PDM microphone data input pin (SDIN2)

// Audio codec configuration
// Note: Your board has no external codec chip (direct I2S connection), use NoAudioCodec
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_30                // Power amplifier enable pin (CTRL)
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_NC                // I2C data line  (no codec chip, set to NC)
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_NC                // I2C clock line (no codec chip, set to NC)
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR  // Codec I2C address (ignored)

/*---------------------------------------------------------------
 * User input
 * The boot button doubles as the chat-state control at runtime.
 *--------------------------------------------------------------*/
#define BOOT_BUTTON_GPIO GPIO_NUM_35  // Boot/function button pin

/*---------------------------------------------------------------
 * Display and MIPI DSI configuration
 * Resolution, lane count, PHY voltage, and orientation must match
 * the physical panel or the screen may remain blank or be distorted.
 *--------------------------------------------------------------*/
#define DISPLAY_WIDTH  1024
#define DISPLAY_HEIGHT 600

#define LCD_BIT_PER_PIXEL (16)
#define PIN_NUM_LCD_RST GPIO_NUM_NC  // LCD reset pin (use NC if not available)

// MIPI DSI configuration (if using MIPI DSI display)
#define DELAY_TIME_MS (3000)
#define LCD_MIPI_DSI_LANE_NUM (2)  // DSI data lane number (usually 2 or 4)

// MIPI DSI power management (LDO channel)
#define MIPI_DSI_PHY_PWR_LDO_CHAN (3)  // LDO channel number
#define MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)  // Voltage (millivolts)

// Display orientation configuration
#define DISPLAY_SWAP_XY false   // Whether to swap XY axes
#define DISPLAY_MIRROR_X false  // X-axis mirror
#define DISPLAY_MIRROR_Y false  // Y-axis mirror
#define DISPLAY_OFFSET_X 0      // X offset
#define DISPLAY_OFFSET_Y 0      // Y offset

// Backlight configuration
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_31  // Backlight control pin
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false  // Whether backlight output is inverted

/*---------------------------------------------------------------
 * Optional GT911 touch configuration
 * The current board constructor leaves touch initialization disabled,
 * but these values define the wiring when that feature is enabled.
 *--------------------------------------------------------------*/
// GT911 touch screen pins
#define TOUCH_GPIO_RST GPIO_NUM_40  // Touch screen reset pin
#define TOUCH_GPIO_INT GPIO_NUM_42  // Touch screen interrupt pin

// Touch screen I2C configuration
#define TOUCH_I2C_SDA_PIN GPIO_NUM_45  // Touch screen I2C SDA pin
#define TOUCH_I2C_SCL_PIN GPIO_NUM_46  // Touch screen I2C SCL pin
#define TOUCH_I2C_PORT I2C_NUM_0       // I2C port number

#endif // _BOARD_CONFIG_H_
