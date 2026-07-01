# Elecrow P4 Board Configuration Guide

## 📋 Step 1: Extract Information from Your Working Code

You mentioned having working code, which is the most important reference! Please follow these steps to extract the information:

### 🔍 Information Checklist

#### 1. **Audio-related Pins** (Search for "I2S" or "GPIO" in your code)

Please find the following pin definitions:
- **MCLK** (Master Clock Pin)
- **BCLK** (Bit Clock Pin)
- **WS/LRCK** (Word Select/Left Right Channel Clock)
- **DIN/SDIN** (Data Input, Microphone)
- **DOUT/SDOUT** (Data Output, Speaker)

**In your code it might look like this:**
```c
#define I2S_MCLK_PIN GPIO_NUM_13
#define I2S_BCLK_PIN GPIO_NUM_12
// ... etc
```

#### 2. **I2C Pins** (Search for "I2C" or "SDA"/"SCL")

Find:
- **SDA** (Data Line)
- **SCL** (Clock Line)

#### 3. **Audio Codec Model** (Search for "ES8311", "ES8374", "ES8388", etc.)

Confirm your codec model, common ones include:
- ES8311
- ES8374
- ES8388
- ES8389

#### 4. **Button Pins** (Search for "BUTTON" or "BOOT")

Find the GPIO number for the boot button or function button

#### 5. **Display Information**

Find:
- **Resolution** (Width and Height, e.g., 800x1280)
- **Display Driver Chip** (e.g., JD9365, ST7703, etc.)
- **MIPI DSI Channel Count** (Usually 2 or 4)
- **DPI Clock Frequency** (e.g., 80MHz)

#### 6. **Flash Size**

Confirm whether your board's Flash is 4MB, 8MB, or 16MB

---

## 📝 Step 2: Modify the config.h File

Open `main/boards/elecrow-p4-board/config.h` and modify according to the information you extracted:

### Example Modification:

Assuming you found from your code:
- MCLK = GPIO_NUM_15
- BCLK = GPIO_NUM_14
- WS = GPIO_NUM_13
- DIN = GPIO_NUM_12
- DOUT = GPIO_NUM_11
- SDA = GPIO_NUM_6
- SCL = GPIO_NUM_7
- Button = GPIO_NUM_0
- Display = 720x720
- Codec = ES8311

**The modified config.h should be:**

```c
// Audio pins
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_15  // Value found from your code
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_13  // Value found from your code
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_14  // Value found from your code
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_12  // Value found from your code
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_11  // Value found from your code

// I2C pins
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_6   // Value found from your code
#define AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_7   // Value found from your code

// Button
#define BOOT_BUTTON_GPIO GPIO_NUM_0  // Value found from your code

// Display resolution
#define DISPLAY_WIDTH 720   // Value found from your code
#define DISPLAY_HEIGHT 720  // Value found from your code
```

---

## 📝 Step 3: Modify the config.json File

Open `main/boards/elecrow-p4-board/config.json` and modify according to your Flash size:

### If Flash is 4MB:
```json
{
    "target": "esp32p4",
    "builds": [
        {
            "name": "elecrow-p4-board",
            "sdkconfig_append": [
                "CONFIG_IDF_TARGET=\"esp32p4\"",
                "CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y",
                "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/4m.csv\""
            ]
        }
    ]
}
```

### If Flash is 8MB:
```json
{
    "target": "esp32p4",
    "builds": [
        {
            "name": "elecrow-p4-board",
            "sdkconfig_append": [
                "CONFIG_IDF_TARGET=\"esp32p4\"",
                "CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y",
                "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME=\"partitions/v2/8m.csv\""
            ]
        }
    ]
}
```

### If Flash is 16MB (default):
Keep the current configuration unchanged

---

## 📝 Step 4: Modify the elecrow_board.cc File

### 4.1 Modify Codec Type

If your codec is **not ES8311**, you need to modify:

**Find this line:**
```cpp
#include "codecs/es8311_audio_codec.h"
```

**Modify according to your codec:**
- ES8374: `#include "codecs/es8374_audio_codec.h"`
- ES8388: `#include "codecs/es8388_audio_codec.h"`
- ES8389: `#include "codecs/es8389_audio_codec.h"`

**Then find the GetAudioCodec() function and modify:**
```cpp
// If ES8374
static Es8374AudioCodec audio_codec(...);

// If ES8388
static Es8388AudioCodec audio_codec(...);
```

### 4.2 Modify Display Driver Chip

If your display driver chip is **not JD9365**, you need to modify:

**Find this line:**
```cpp
#include "esp_lcd_jd9365_10_1.h"
```

**Modify according to your chip:**
- ST7703: `#include "esp_lcd_st7703.h"`
- Other chips: Check the header file used in your code

**Then find the relevant code in the InitializeLCD() function and modify:**
```cpp
// If ST7703
esp_lcd_dsi_bus_config_t bus_config = ST7703_PANEL_BUS_DSI_2CH_CONFIG();
esp_lcd_dbi_io_config_t dbi_config = ST7703_PANEL_IO_DBI_CONFIG();
esp_lcd_new_panel_st7703(io, &lcd_dev_config, &disp_panel);
```

### 4.3 Modify DPI Clock Frequency

In the `InitializeLCD()` function, find:
```cpp
.dpi_clock_freq_mhz = 80,  // Clock frequency
```

Modify this value according to your display specifications (common values: 46, 80, 100, etc.)

### 4.4 Touchscreen Support

If your board **does not support touchscreen**, comment out in the constructor:
```cpp
// InitializeTouch();  // Already commented, if touchscreen is supported, uncomment
```

If touchscreen is supported but the touch chip is not GT911, you need to modify the touch initialization code.

---

## 🧪 Step 5: Test Compilation

### 5.1 Set Build Target

```bash
idf.py set-target esp32p4
```

### 5.2 Build the Project

```bash
idf.py build
```

### 5.3 Check Compilation Errors

If there are compilation errors, please:
1. **Read the error messages carefully**
2. **Check if all pin numbers are correct**
3. **Check if the codec and display driver match**

---

## 🔧 Troubleshooting Common Issues

### Issue 1: Header File Not Found

**Error Example:**
```
fatal error: codecs/es8311_audio_codec.h: No such file or directory
```

**Solution:**
- Check if the codec type is correct
- Check which codecs are available in the `main/audio/codecs/` directory

### Issue 2: GPIO Definition Error

**Error Example:**
```
GPIO_NUM_XX is not declared
```

**Solution:**
- Check if the GPIO number is within the valid range for ESP32-P4
- Confirm if the GPIO number is correct

### Issue 3: Display Not Showing

**Possible Causes:**
- Display driver chip mismatch
- Incorrect DPI clock frequency
- Incorrect MIPI DSI channel count
- Incorrect resolution configuration

### Issue 4: No Audio Output

**Possible Causes:**
- Incorrect I2S pin configuration
- Incorrect codec I2C address
- Incorrect PA enable pin configuration

---

## 📞 Information to Provide When Seeking Help

If you encounter problems, please provide:

1. **Your board model and specifications**
2. **Pin configuration extracted from your working code**
3. **Compilation error messages (complete output)**
4. **Your hardware configuration:**
   - Codec model
   - Display model and resolution
   - Flash size
   - Whether touchscreen is supported

---

## ✅ Checklist

Before flashing, please confirm:

- [ ] All GPIO pin numbers have been extracted from your code and modified
- [ ] Codec type has been correctly configured
- [ ] Display driver chip has been correctly configured
- [ ] Display resolution has been correctly configured
- [ ] Flash size has been correctly configured
- [ ] Compilation passes without errors
- [ ] Touchscreen configuration (if not needed, has been commented out)

---

## 🚀 Next Steps

After configuration is complete, use the following commands to compile and flash:

```bash
# Method 1: Using script (recommended)
python scripts/release.py elecrow-p4-board

# Method 2: Manual compilation
idf.py set-target esp32p4
idf.py build
idf.py flash monitor
```

Good luck with your configuration! If you have any questions, feel free to ask.
