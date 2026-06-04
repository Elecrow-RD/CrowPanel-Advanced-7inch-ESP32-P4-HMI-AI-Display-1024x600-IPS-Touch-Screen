#pragma once

/*********************** Pin define ***********************/
#define AUDIO_GPIO_CTRL         (30)    // GPIO pin number for audio power
#define AUDIO_POWER_ENABLE      (LOW)   // GPIO set low level to enable audio power
#define AUDIO_POWER_DISABLE     (HIGH)  // GPIO set high level to disable audio power

#define AUDIO_GPIO_LRCLK        (21)    // GPIO pin number for LRCLK (Left-Right Clock  of I2S)
#define AUDIO_GPIO_BCLK         (22)    // GPIO pin number for BCLK (Bit Clock of I2S)
#define AUDIO_GPIO_SDATA        (23)    // GPIO pin number for SDATA (Serial Data of I2S)

#define MIC_GPIO_CLK            (24)    // GPIO pin number for microphone BCLK (Bit Clock of PDM)
#define MIC_GPIO_SDIN           (26)    // GPIO pin number for microphone SDIN (Serial Data of PDM)
/*********************** Pin define ***********************/
