# ESP32-P4 7.0-Inch Display Product Hardware Driver Guide

| Document Attribute | Details |
| --- | --- |
| Document Version | V1.0 |
| Applicable Hardware | ESP32-P4 Display 7.0 inch V1.2 |
| Baseline Date | 2026-07-29 |
| Schematic Date | 2026-05-05 (drawing frame date) |
| Author | OpenAI Codex (compiled from project schematics and validated examples) |
| Document Status | Suitable for hardware maintenance, driver porting, and onboarding handoff |

## 1. Document Scope and Evaluation Criteria

This document cross-checks the following evidence within the project:

1. `1.2/ESP32-P4 Display 7.0 inch V1.2.sch`: EAGLE 9.6.2 schematic source file, used as the primary evidence for components, nets, and electrical connections.
2. `1.2/ESP32-P4 Display 7.0 inch V1.2.pdf`: Single-page full-board schematic, used as visual verification evidence for functional sections, populated/NC markings, and graphical connections.
3. `idf-code/Lesson01` through `Lesson17`: ESP-IDF examples organized by function, used as the primary evidence for working software configurations.
4. The `idf_component.yml`, `Kconfig`, header files, and BSP source files for each example: used as evidence for versions, default values, and build dependencies.

The evidence priority is as follows:

`Effective constants/initialization parameters in working code` > `Kconfig defaults or comments in the same directory` > `V1.2 schematic nets` > `general README`.

When the code and schematic are inconsistent, this document uses the code as the current runtime baseline as required and lists the discrepancies in Section 15. Here, “validated” means that the repository provides the function as a standalone lesson example; it does not mean that the hardware was reconnected and electrically tested during the generation of this document. The repository does not include complete `sdkconfig` files, build artifacts, or test logs, so final values that depend on menuconfig must still be fixed in the production project.

## 2. Product Hardware Architecture

The product uses the ESP32-P4NRW32 as the main application controller, while the ESP32-C6-MINI-1-N4 provides Wi-Fi/wireless coprocessing capabilities. The main controller connects directly to the 7-inch MIPI-DSI display, MIPI-CSI camera interface, touch I2C, SDMMC, PDM microphone, I2S amplifiers, USB 2.0, and UART/I2C/GPIO expansion interfaces. The board also includes an STC8H1K08 for auxiliary control functions such as charging-status and battery monitoring.

The main onboard power paths include USB/5 V input, battery charging and power switching, 5 V-to-3.3 V conversion, 5 V-to-1.2 V conversion, a 3.3 V audio LDO, 2.8 V/1.8 V camera LDOs, LCD power, and a boost/constant-current backlight driver.

## 3. Peripheral Overview

| Category | Device/Interface | Key Connection or Model | Software Validation Status |
| --- | --- | --- | --- |
| Main controller | ESP32-P4NRW32 (U7) | External W25Q128JVSIQ, 40 MHz, 32.768 kHz | Validated; target of all ESP-IDF examples |
| Wireless coprocessor | ESP32-C6-MINI-1-N4 (IC1) | P4 GPIO47/48 UART; GPIO32 enable; C6 SDIO network present | Wi-Fi example available; underlying P4-C6 transport abstraction is not fully included in the repository |
| Display | 7.0-inch 1024x600 IPS + EK79007 | 2-lane MIPI-DSI, GPIO31 backlight | Validated |
| Touch | GT911 capacitive touch | I2C0 GPIO45/46, RST40, INT42 | Validated |
| Camera | SC2336 (software detection target)/24-pin CSI FPC | 2-lane MIPI-CSI, SCCB GPIO12/13 | Validated |
| Storage | W25Q128JVSIQ NOR Flash | Dedicated ESP32-P4 Flash pins | Boot storage, managed by ESP-IDF |
| Storage | MicroSD card socket | Schematic: SDMMC GPIO14..19; code: GPIO43/44/39 | Validated code differs from schematic; see Section 15 |
| Audio input | LMA3526B381-class PDM microphone (U176) | CLK24, DATA26, L/R20 | Validated |
| Audio output | Dual NS4168 (U13/U3) + dual speakers | I2S GPIO21/22/23, SD GPIO30 | Validated |
| Optional audio codec | ES7210 ADC, ES8311 DAC | Explicitly marked `NC` in schematic | Must not be enabled by default |
| USB Device | ESP32-P4 USB 2.0 D+/D- | Native TinyUSB HID | Validated |
| USB-to-UART | CH340K (U1) | Onboard download/debug channel | Fixed hardware function; no application driver |
| UART expansion | UART1/UART2/UART3 nets | GPIO47/48, 34/33, 27/28, etc. | UART2 code validated |
| I2C expansion | 4-pin I2C_OUT | GPIO45/46 level-shifted to 3.3 V through BSS138 | Bus driver validated |
| GPIO expansion | 2x12 J7 | GPIO2/3/4/5/25/49..54, 3.3 V/5 V/GND | No unified driver; available for peripheral reuse |
| Environmental sensor | DHT20 (external) | I2C 0x38, shared GPIO45/46 | Example validated; not an onboard device |
| LoRa | SX1262 (external) | SPI GPIO6/7/8 + control pins | TX/RX examples validated; control-pin version conflict exists |
| 2.4 GHz | nRF24 (external) | SPI GPIO6/7/8 + control pins | TX/RX examples validated; control-pin version conflict exists |
| Charging management | TP4059-SOT23-6 (U2) | 5 V, battery, CHG/STDBY status | Automatic hardware function; monitored by STC8 |
| Auxiliary MCU | STC8H1K08-36I (U14) | Charging LED, status, ADC_VBAT, UART | Present in schematic; repository contains no STC8 firmware |
| Power | MT3406 x2, MT9201, MT3608, ME6211 series | 3.3 V, 1.2 V, LCD/backlight, camera/audio power | Automatic hardware function/enable-pin controlled |
| Buttons/switches | BOOT K3, RESET K4, power slide switch SW1 | BOOT/CHIP_PU/power MOSFET | Fixed hardware function |
| Indicators | Charging/full/power LEDs | TP4059 + STC8/discrete logic | Repository contains no application-level LED driver |

## 4. ESP32-P4 GPIO Resource Table

The table below prioritizes the values actually used by the code; “Schematic Function” is provided to identify electrical connections.

| GPIO | Current Code Function | Direction/Multiplexing | Schematic Net/Electrical Connection | Conflict Notice |
| ---: | --- | --- | --- | --- |
| 6 | Wireless SPI MOSI | SPI output | Wireless interface series resistor | Shared by wireless modules |
| 7 | Wireless SPI MISO | SPI input | Wireless interface series resistor | Shared by wireless modules |
| 8 | Wireless SPI SCK | SPI output | Wireless interface series resistor | 8 MHz |
| 9 | SX1262 BUSY / nRF24 IRQ | Input/interrupt | Wireless interface | The two modules are mutually exclusive |
| 10 | SX1262 NSS | Push-pull output | Wireless interface | nRF24 code does not use this pin |
| 11 | Camera RESET net | GPIO output through level shifter | `IO11_CSI_RESET` | Current camera code instead configures `reset_pin=-1` |
| 12 | Camera SCCB SDA | I2C open-drain | `I2C2_SDA` through BSS138 | Code enables internal pull-up |
| 13 | Camera SCCB SCL | I2C open-drain | `I2C2_SCL` through BSS138 | 100 kHz |
| 14..19 | Onboard SDMMC D3/D2/D1/D0/CLK/CMD in schematic | SDMMC | Directly connected to SD card nets | Not used by current SD code |
| 20 | PDM microphone L/R selection | GPIO/strap | `MIC_L/R` | Current code does not actively set it; channel is determined by hardware resistor |
| 21 | Audio LRCLK | I2S1 output | Dual NS4168 LRCLK | 16 kHz |
| 22 | Audio BCLK | I2S1 output | Dual NS4168 BCLK | Standard I2S |
| 23 | Audio SDATA | I2S1 output | Dual NS4168 SDATA | 16-bit stereo |
| 24 | PDM CLK | I2S0 output | PDM microphone CLK; also routed to optional codec MCLK | Occupied during recording |
| 26 | PDM DATA | I2S0 input | PDM microphone DATA | Occupied during recording |
| 27 | SX IRQ / UART TX (code) | Interrupt input/output | Schematic net is `P4_TXD2` | Conflicts with GPIO53 Kconfig default |
| 28 | SX RESET / nRF CS / UART RX (code) | Output/input | Schematic net is `P4_RXD2` | Multiple functions are mutually exclusive; conflicts with GPIO54 Kconfig default |
| 29 | LCD backlight power control | GPIO output | `LCD_BK_POWER`, Q11 marked NC | Not driven by current display code |
| 30 | Audio amplifier SD | Push-pull output, active-low enable | `AUDIO_OUT_SD` -> Q10 -> dual amplifier CTRL | Validated |
| 31 | LCD backlight brightness | LEDC PWM | `LCD_BK_EN` -> MT9201 EN | 30 kHz, 11-bit |
| 32 | ESP32-C6 EN | Push-pull output/reset | `C6_EN` | Requires attention when powering up the Wi-Fi subsystem |
| 33 | Expansion UART RX | UART input | `RXD3` through level shifter | UART_IN_RXD3 on the 5 V side |
| 34 | Expansion UART TX | UART output | `TXD3` through level shifter | UART_IN_TXD3 on the 5 V side |
| 37 | Debug/download TXD0 | UART output | `TXD0` | CH340K path |
| 38 | Debug/download RXD0 | UART input | `RXD0` | CH340K path |
| 39 | SD D0 (code) | SDMMC input/output | Schematic only shows `P4_IO39` through series resistor | Inconsistent with onboard SD schematic |
| 40 | GT911 RESET | Push-pull output, active-low | `RESET_TP` | Validated |
| 41 | LCD RESET | Push-pull output, active-low | `LCD_RESET` | Display driver sets `reset_gpio_num=-1`; handled by hardware/panel path |
| 42 | GT911 INT | Input/touch-address strap | `INT_TP` | Driver interrupt level configured as 0 |
| 43 | SD CLK (code) | SDMMC output | Schematic only shows `P4_IO43` | Inconsistent with onboard SD schematic |
| 44 | SD CMD (code) | SDMMC bidirectional | Schematic only shows `P4_IO44` | Inconsistent with onboard SD schematic |
| 45 | GT911/expansion I2C SDA | I2C0 open-drain | `I2C1_SDA`, through BSS138 to the 3.3 V side | Shared bus |
| 46 | GT911/expansion I2C SCL | I2C0 open-drain | `I2C1_SCL`, through BSS138 to the 3.3 V side | 400 kHz |
| 47 | ESP32-C6/UART2 TX | UART output | `TXD1` | 115200 8N1 example |
| 48 | ESP32-C6/UART2 RX | UART input | `RXD1` | 115200 8N1 example |
| 49..52 | J7 GPIO expansion | General-purpose GPIO | Directly connected to J7 | Pay attention to external loads |
| 53/54 | J7 GPIO / default wireless Kconfig control pins | General-purpose GPIO | Directly connected to J7 | Actual code headers do not use Kconfig values |

MIPI DSI, MIPI CSI, USB D+/D-, external Flash, crystal, and ESP32-P4 internal power pins are dedicated pins. They are not included in the general-purpose GPIO table and must not be repurposed as general-purpose I/O.

## 5. Main Controller, Boot, and External Flash

### 5.1 ESP32-P4NRW32

- Main controller: U7, ESP32-P4NRW32, RISC-V application processor.
- Software layer: ESP-IDF + FreeRTOS; the display/touch functions use Espressif Component Manager components.
- Minimum dependency: Multiple examples specify ESP-IDF `>=5.4.0`; the display lesson specifies `>=5.4.2`.
- Boot: K3 pulls `SPI_BOOT` low to enter download mode; K4 pulls `CHIP_PU` low to reset.
- Clocks: 40 MHz main crystal; 32.768 kHz low-speed crystal.

### 5.2 W25Q128JVSIQ

- The dedicated U7 `FLASH_CS/CK/WP/HOLD/D/Q` pins connect to the IC10 W25Q128JVSIQ.
- The driver is managed by the ESP-IDF ROM bootloader, SPI Flash driver, and partition table. It must not be reinitialized at the application layer using a general-purpose SPI driver.
- Partitions: Each lesson project includes its own `partitions.csv`; when porting to a unified product project, a single partition baseline must be selected.
- Risk: Do not reuse dedicated Flash pins. Before changing the Flash frequency/mode, verify `menuconfig` and the device capabilities.

## 6. Display and Backlight

### 6.1 EK79007 MIPI-DSI Display

| Parameter | Validated Code Value |
| --- | --- |
| Resolution | 1024 x 600 |
| Pixel format | RGB565, 16 bpp |
| DSI | bus 0, 2 data lanes, 900 Mbps/lane |
| DPI pixel clock | 51 MHz |
| Horizontal timing | BP 160, pulse 70, FP 160 |
| Vertical timing | BP 23, pulse 10, FP 12 |
| DBI commands | 8-bit command, 8-bit parameter, VC0 |
| Frame buffer | 1 panel FB; LVGL uses double buffering in SPIRAM |
| Rotation | No XY swap or mirroring |

Software dependencies: `esp_lcd_mipi_dsi`, `espressif/esp_lcd_ek79007 ^1.0.2`, `espressif/esp_lvgl_port ^2.6.0`, `lvgl ^8.3.11`, FreeRTOS.

Key configuration example:

```c
esp_lcd_dsi_bus_config_t bus = {
    .bus_id = 0,
    .num_data_lanes = 2,
    .lane_bit_rate_mbps = 900,
};
esp_lcd_dpi_panel_config_t dpi = {
    .dpi_clock_freq_mhz = 51,
    .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
    .video_timing = { .h_size = 1024, .v_size = 600,
        .hsync_back_porch = 160, .hsync_pulse_width = 70, .hsync_front_porch = 160,
        .vsync_back_porch = 23, .vsync_pulse_width = 10, .vsync_front_porch = 12 },
};
```

Initialization sequence: backlight PWM initialization -> DSI bus -> DBI IO -> EK79007 panel reset/initialization -> LVGL port -> first-frame rendering -> turn on backlight. The code explicitly keeps the backlight off at the end of `display_init()`; the caller must subsequently invoke `set_lcd_blight()`.

### 6.2 Backlight

- GPIO31, GPIO push-pull + LEDC low-speed channel 0, timer 0.
- 30 kHz, 11-bit, PLL divided clock.
- The brightness interface accepts 0..100; the actual nonzero duty is `brightness * 18 + 200`, so it is not a strict percentage.
- Electrical path: GPIO31 -> MT9201 EN/dimming path -> DSI LED driver; the schematic specifies an LED current of approximately 182 mA (`200 mV / 1.1 ohm`).
- Risk: Do not configure GPIO31 as a regular GPIO held high for extended periods and thereby bypass the PWM constraints. Changing the backlight current requires modifying the hardware sense resistor, not merely changing the duty cycle.

## 7. GT911 Touch

- GPIO45 SDA, GPIO46 SCL, I2C master port 0, 7-bit address.
- Bus frequency: 400 kHz; I2C glitch filter: 7 cycles. The code enables the SoC internal pull-ups, while the schematic also includes level shifting/external pull-up networks.
- GPIO40 RESET, active-low; GPIO42 INT, with the driver configured for interrupt level=0.
- Resolution bounds: 1024x600; no XY swap or mirroring.
- Address: First uses `ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS`; if that fails, it tries `..._ADDRESS_BACKUP`, supporting both GT911 address-strap states.
- Software layer: ESP-IDF new I2C master API, `esp_lcd_panel_io_i2c`, `esp_lcd_touch_gt911`, FreeRTOS.

Initialization sequence: `i2c_init()` -> create panel I/O -> `esp_lcd_touch_new_i2c_gt911()` -> periodically call `touch_read()`. The USB HID example reads at 10 ms intervals, corresponding to an application sampling rate of 100 Hz.

Note: Before switching to the backup address after a failure, the current code does not delete the panel I/O handle created during the first attempt. For long-term maintenance, unified failure cleanup is recommended; this issue does not change the existing working parameters.

## 8. MIPI-CSI Camera

### 8.1 Electrical Connections

- 24-pin FPC3, 2-lane MIPI-CSI: D0+/D0-, D1+/D1-, CLK+/CLK-.
- SCCB: GPIO12 SDA, GPIO13 SCL, through BSS138 level shifting.
- Schematic: GPIO11 connects to `CSI_RESET` through a level shifter; the FPC also provides XVCLK, AVDD_2V8, and DOVDD_1V8.
- Software identification/tuning file: SC2336, 50 Hz anti-flicker.

### 8.2 Driver Configuration

- SCCB master port 1, 100 kHz, with internal pull-ups enabled.
- `esp_video_init()` reuses the previously created I2C bus handle.
- `reset_pin=-1`, `pwdn_pin=-1`; the current code does not operate the GPIO11 reset shown in the schematic.
- V4L2 device: `ESP_VIDEO_MIPI_CSI_DEVICE_NAME`, opened as nonblocking and read-only.
- Output format: forced to RGB565; the resolution is negotiated by the current sensor/V4L2 mode and is not hard-coded in the BSP.
- Two cache-aligned SPIRAM user-pointer buffers using V4L2 `REQBUFS/QUERYBUF/QBUF`; the capture task is pinned to core 0 at priority 3.
- Software dependencies: `esp_cam_sensor ^1.2.0`, `esp_sccb_intf ^0.0.5`, `esp_video ^1.1.0`, V4L2, LVGL, SPIRAM.

Initialization sequence: SCCB bus -> `esp_video_init()` -> open CSI device -> query/set RGB565 -> allocate and queue double buffers -> create LVGL canvas -> stream on.

Risk: Camera power and reset timing currently depend on board-level hardware/default driver behavior. If the sensor is replaced or intermittent detection failures occur during cold startup, add the GPIO11 reset sequence and a power-stabilization delay before attempting to increase the SCCB frequency.

## 9. SD Card

### 9.1 Current Working Code Baseline

- SDMMC host slot 0.
- GPIO43 CLK, GPIO44 CMD, GPIO39 D0.
- 1-bit mode, 10 MHz, internal pull-ups enabled.
- FAT VFS mount point `/sdcard`; maximum of 5 files; allocation unit 16 KiB; no automatic formatting on mount failure.

```c
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
host.slot = SDMMC_HOST_SLOT_0;
host.max_freq_khz = 10000;
sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
slot.clk = GPIO_NUM_43;
slot.cmd = GPIO_NUM_44;
slot.d0 = GPIO_NUM_39;
slot.width = 1;
slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
```

### 9.2 Schematic Baseline

The V1.2 schematic labels the onboard SD card signals as GPIO14 D3, GPIO15 D2, GPIO16 D1, GPIO17 D0, GPIO18 CLK, and GPIO19 CMD, and connects them through series resistors/filter networks to the ESP32-C6 SD2 GPIO18..23. The code clearly differs from the schematic; see Section 15 for details.

Maintenance principle: Do not directly change the working code to GPIO17/18/19 until waveform and continuity testing on the physical board has been completed. If the target is the V1.2 onboard card socket, also verify the PCB `.brd`, card-socket nets, and board-revision rework before determining the production configuration.

## 10. Audio

### 10.1 PDM Microphone

- Device: U176, LMA3526B381-class digital microphone. The schematic notes both analog and digital microphone population options; the current code uses the digital PDM path.
- I2S0 master RX; GPIO24 CLK, GPIO26 DIN, GPIO20 L/R determined by hardware resistor.
- 16 kHz, 16-bit, mono, left slot; 8x downsampling; BCLK divider 8; MCLK multiple 256.
- High-pass filter enabled with a 35.5 Hz cutoff; amplify 1.
- DMA: 6 descriptors x 256 frames.
- Software layer: ESP-IDF `driver/i2s_pdm.h`, FreeRTOS, and SPIRAM.


### 10.2 I2S Amplifiers and Speakers

- Two NS4168 devices drive the two speaker channels independently. The devices receive standard I2S directly and require no I2C register initialization.
- I2S1 master TX: GPIO21 LRCLK, GPIO22 BCLK, and GPIO23 SDATA.
- 16 kHz, 16-bit, stereo, both slots, WS width 16, bit shift enabled, left-aligned.
- No MCLK output; DMA uses 6 x 256 frames.
- GPIO30 `AUDIO_OUT_SD` is the global amplifier enable. The code defines it as active-low: `true` writes 0, and `false` writes 1.

Initialization sequence: call `audio_ctrl_init()` and keep the amplifiers disabled -> call `audio_init()` -> write valid PCM/silent frames -> call `set_Audio_ctrl(true)` to prevent startup pops.

### 10.3 Unpopulated Codecs

ES7210 (IC5) and ES8311 (IC6) are marked `_NC` in the schematic, and their associated I2C/I2S networks are reserved only. Do not load codec drivers merely because the component symbols appear in the schematic. If a future BOM revision populates these devices, I2C register initialization, power sequencing, and MCLK must be added, and I2S pin multiplexing with PDM/NS4168 must be handled.

## 11. Communication Interfaces

### 11.1 Native USB 2.0 Device

- The dedicated ESP32-P4 DP/DM pins connect directly to the USB interface network; the application layer does not configure the GPIO matrix.
- The verified example uses a TinyUSB HID mouse: one HID interface, IN endpoint `0x81`, 16-byte maximum packet, and a 10 ms polling interval.
- The descriptors declare 100 mA and remote wakeup; the internal PHY is used, and FS/HS share the same HID configuration descriptor.
- Software layer: ESP-IDF TinyUSB device driver.

Risk: The product’s actual maximum current draw, including battery charging and peak backlight load, may clearly exceed the 100 mA declared in the HID descriptor. This value is only from the example USB descriptor. For a compliant product, the declared current must be updated according to the actual power architecture.

### 11.2 CH340K USB-to-UART

- U1 CH340K works with the U8 transistor/automatic download network to connect ESP32-P4 UART0 to BOOT/RESET.
- This is a PC-side USB-to-UART chip and does not require an ESP32-P4 application driver.
- Debug UART network: GPIO37 TXD0 and GPIO38 RXD0. If flashing fails, check K3/K4, the CH340 driver, and the automatic download transistor network.

### 11.3 UART Expansion and ESP32-C6 Channel

The verified `bsp_uart` configuration uses UART2: GPIO47 TX, GPIO48 RX, 115200 baud, 8N1, no flow control, default source clock, and a 2 KiB RX buffer. Comments indicate that it is used for the Wi-Fi module, and GPIO47/48 also correspond to the TXD1/RXD1 path in the schematic.

GPIO34/33 also connect to a 5 V UART input expansion interface through BSS138 level shifters. The wireless BSP additionally provides UART1 transparent mode on GPIO27/28. Do not mix up the UART numbers, GPIO assignments, or physical connections of these three UART groups.

### 11.4 I2C Expansion

- I2C0: GPIO45 SDA, GPIO46 SCL, 400 kHz, shared by GT911 and `I2C_OUT` through BSS138 level shifting.
- I2C1/SCCB: GPIO12 SDA, GPIO13 SCL, 100 kHz, dedicated to the camera.
- External DHT20 example: I2C0, 7-bit address `0x38`; trigger command `AC 33 00`, wait at least 80 ms, 7-byte data, CRC-8 polynomial 0x31, and a 1 s timeout.
- Risk: The DHT20 is an external course component and is not included in the V1.2 mainboard schematic BOM. Before connecting it, verify the voltage on the 3.3 V side of `I2C_OUT` and ensure its address does not conflict with the touchscreen controller.

## 12. Wi-Fi, LoRa, and nRF24

### 12.1 ESP32-C6 Wi-Fi

The ESP32-P4 itself does not include 2.4 GHz Wi-Fi. The onboard IC1 is an ESP32-C6-MINI-1-N4. Schematic connections include:

- GPIO32 -> C6_EN.
- GPIO47/48 -> UART channel.
- P4 GPIO14..19 and C6 GPIO18..23 form the SDIO-related network.
- C6 GPIO9 is the boot pin, and a C6 UART0 download network is also provided.

Lesson16/17 use the standard `esp_wifi`, `esp_netif`, event loop, and NVS, with support for STA/AP/AP+STA. The repository does not clearly include a complete transport/remote Wi-Fi adaptation layer between the P4 host and C6 slave. Therefore, when porting, do not assume that the P4 can directly use the C6 radio simply by copying `bsp_wifi.c`. Retain the target chip, managed components, and menuconfig combination from the original course project, and confirm the actual build target.

### 12.2 External SX1262 LoRa Module

| Parameter | Code Value |
| --- | --- |
| SPI | SCK8, MISO7, MOSI6, 8 MHz |
| Control | BUSY9, IRQ27, NRST28, NSS10 (effective header file) |
| RF | 915.0 MHz |
| Bandwidth/Spreading Factor | 125 kHz / SF7 |
| Coding rate | 7 |
| Sync word | RadioLib private |
| Power | 22 dBm |
| Preamble | 8 symbols |
| TCXO voltage parameter | `1.6` (last parameter of RadioLib `begin()`) |
| Driver | RadioLib + custom EspHal + ESP-IDF SPI/GPIO ISR |

IRQ uses interrupt callbacks to transition the TX/RX state. The 22 dBm setting is a high-power configuration; verify the external module’s power supply, antenna, regional regulatory compliance, and thermal performance.

### 12.3 External nRF24 Module

- Shares SPI GPIO6/7/8 at 8 MHz.
- Effective header file: IRQ9, CE27, CS28.
- 2400 MHz, 250 kbps, address width 5.
- TX pipe: `01 02 11 12 FF`.
- Driver: RadioLib nRF24 + custom EspHal.

The SX1262, nRF24, and UART transparent mode share control pins, so only one path can be enabled in a build. The Kconfig defaults do not match the hard-coded values in the header file; see Section 15.

## 13. Power, Charging, Buttons, and Indicators

### 13.1 Charging and Battery

- U2 TP4059: Uses a 5 V input to charge a single-cell lithium battery; the CHG/STDBY outputs form `POWER_CHRG/POWER_DONE`.
- U14 STC8H1K08 reads the charging status, controls the red/green LEDs, and monitors the battery through the `ADC_VBAT` resistor divider.
- SW1 and Q3/Q5 NCE20P45Q form the power-path/switch control.
- The repository does not include the STC8 firmware, so the ADC calibration, LED polarity, and charging state machine cannot be confirmed from the code. Maintenance must follow the original STC8 firmware and must not infer these details from the ESP32-P4 code.

Lithium battery risk: Before replacing the battery, charging-current setting resistor, or power MOSFET, repeat temperature-rise, overcharge/short-circuit, and reverse-polarity testing. Software cannot replace a battery protection circuit.

### 13.2 Power Rails

| Power Block | Device | Purpose/Control |
| --- | --- | --- |
| 5 V -> 3.3 V | MT3406 (U9) | Main 3.3 V rail, hardware feedback |
| 5 V -> 1.2 V | MT3406 (U10) | 1.2 V rail, hardware feedback |
| LCD power | ME6211/associated load switch | Panel power |
| Backlight | MT9201 (U6) | GPIO31 PWM/EN, constant-current boost |
| LED boost | MT3608 (U16) | LCD LED driver power path |
| Camera rails | ME6211C28 (IC3), etc. | AVDD 2.8 V, DOVDD 1.8 V |
| Audio 3.3 V | ME6211C33 (IC7/IC8) | Audio/auxiliary MCU power |

Most power devices are controlled by hardware feedback and EN networks and have no register drivers. When porting drivers, preserve the sequence “power stable -> release reset -> initialize buses -> enable loads.”

### 13.3 BOOT, RESET, and LEDs

- K3: Pulls `SPI_BOOT` low when pressed to enter download mode.
- K4: Pulls `P4_RST_EN/CHIP_PU` low when pressed to reset the main controller.
- The power/charging LEDs are controlled by the TP4059, STC8, and discrete circuitry. The ESP32-P4 examples do not provide a reliable GPIO mapping for an onboard LED. Lesson02 is a generic GPIO example and cannot be used directly as the basis for this board’s LED driver.

## 14. Software Layers and Recommended Initialization Sequence

### 14.1 Software Layers

| Layer | Components |
| --- | --- |
| OS/Foundation | ESP-IDF >=5.4.x, FreeRTOS, NVS, event loop |
| HAL/Drivers | GPIO, LEDC, I2C new master, I2S std/PDM, SDMMC, TinyUSB, MIPI DSI/CSI |
| Managed components | EK79007, esp_lvgl_port, LVGL, esp_cam_sensor, esp_sccb_intf, esp_video |
| Third-party | RadioLib (SX1262/nRF24) |
| File/Media | FAT VFS, V4L2, LVGL canvas, SPIRAM heap |

The code does not directly manipulate bare-metal registers. It primarily uses the ESP-IDF HAL/drivers and FreeRTOS; wireless modules use RadioLib wrappers around ESP-IDF SPI/GPIO.

### 14.2 Recommended Full-System Initialization Sequence

1. Initialize NVS, logging, the required 3.3 V/1.8 V/2.8 V power supplies, and stabilization delays.
2. Keep the backlight and audio amplifiers disabled.
3. Initialize the ESP32-C6 enable/transport path only when Wi-Fi is required.
4. Initialize I2C0, followed by GT911 and other shared I2C devices.
5. Initialize MIPI-DSI, EK79007, and LVGL; after drawing the first frame, gradually enable the GPIO31 backlight.
6. Initialize SCCB and the camera; perform a GPIO11 reset if necessary, then start the V4L2 stream.
7. Initialize SDMMC and mount FAT; begin validation with a conservative 10 MHz/1-bit configuration.
8. Initialize I2S/PDM; send silent data first, then enable the active-low GPIO30 amplifier control.
9. Initialize the external SX1262/nRF24 or expansion UART last to prevent glitches on multiplexed GPIOs during early startup.

## 15. Schematic and Code Discrepancy List

| ID | Item | Verified Code | V1.2 Schematic/Kconfig | Current Conclusion and Possible Cause |
| --- | --- | --- | --- | --- |
| D-01 | SDMMC | CLK43/CMD44/D0=39, 1-bit | Onboard SD uses CLK18/CMD19/D0=17 and also includes D1..D3=16..14 | As required, use the code as the operational baseline. Possible causes include board-revision rerouting that was not synchronized, an example intended for an external/older card slot, or incorrect schematic net labels. The V1.2 onboard card slot must be verified on actual hardware |
| D-02 | SX1262 IRQ/RESET | 27/28 (actual macros in header file) | Kconfig defaults 53/54; J7 exposes 53/54 | The preprocessor uses the hard-coded header values, so the Kconfig values are currently ineffective. The code may be inherited from an older board or a different wiring sequence; the operational baseline is 27/28 |
| D-03 | nRF24 CE/CS | 27/28 (actual macros in header file) | Kconfig defaults 53/54 | Same as D-02; the operational baseline is 27/28 |
| D-04 | Wireless UART TX/RX | 27/28 (header file) | Kconfig defaults 53/54 | Internally inconsistent within the same BSP and mutually exclusive with the RF control pins; follow the header file |
| D-05 | Camera reset | `reset_pin=-1` | GPIO11 connects to CSI_RESET through level shifting | The verified code relies on the hardware default/sensor power-on reset; the reserved schematic control is unused |
| D-06 | LCD reset | `reset_gpio_num=-1` | GPIO41 connects to LCD_RESET | The EK79007 driver/panel power-on path does not use this GPIO. Preserve the current baseline and add timing only if cold-start issues occur |
| D-07 | LCD backlight power | Only drives GPIO31 | The schematic also includes GPIO29 `LCD_BK_POWER`, corresponding to Q11 marked NC | This power-control path is either unpopulated or does not require software control; do not drive GPIO29 without verification |
| D-08 | Wi-Fi driver location | Lesson16/17 directly use `esp_wifi` | ESP32-P4 has no Wi-Fi; onboard ESP32-C6 | The examples may be built for the C6 or depend on a host/slave layer not included in the repository. A unified product project must confirm the target/transport |
| D-09 | Audio codec | Code sends I2S directly to NS4168/PDM mic | ES7210/ES8311 appear in the schematic but are marked NC | Not a conflict; these are unpopulated optional solutions, so codec drivers must not be loaded |

Conditions for closing discrepancies: Record the board serial number and PCB revision; verify nets using a multimeter/wiring map; verify timing using an oscilloscope/logic analyzer; finalize `sdkconfig.defaults`; and synchronize confirmed results back to the schematic, BSP header files, and Kconfig.

## 16. Risks/Precautions

1. **GPIO multiplexing conflicts**: GPIO27/28 are simultaneously assigned to SX1262, nRF24, and wireless UART; GPIO6/7/8 are shared SPI pins. Only one external wireless solution can be enabled.
2. **SD board-revision risk**: The code and schematic directly conflict. Driving the wrong pins may leave the card unresponsive or cause conflicts with external GPIO outputs.
3. **Voltage-level risk**: ESP32-P4 I/O uses 3.3 V logic and is not 5 V tolerant. The 5 V UART/I2C expansion interfaces depend on the onboard BSS138/level-shifting networks and must not be connected directly while bypassing the level shifters.
4. **MIPI signal integrity**: DSI/CSI use high-speed differential lines. Do not use jumper wires, long stubs, or heavily loading standard GPIO probes. The FPC model, orientation, and impedance must match.
5. **Backlight and audio drive capability**: GPIOs only drive EN/logic inputs and cannot directly drive the LED backlight, speakers, or relays.
6. **USB power declaration**: The HID example declares 100 mA, but peak current from the full system’s backlight, charging, and audio may be higher. Productization requires recalculating USB current and the enumeration strategy.
7. **I2C pull-ups**: The code enables internal pull-ups, while the board also includes external pull-ups/level shifting. Excessively strong parallel pull-ups increase low-level sink current. Before connecting external modules, measure the effective resistance and rise time.
8. **Active-low amplifier control**: GPIO30 uses inverted polarity. The initialization stage must first write the disabled state to prevent pops and high current.
9. **Misuse of NC components**: ES7210, ES8311, the GPIO29 path corresponding to Q11, and other paths are marked NC. Software configuration has no effect when these components are not populated in the BOM.
10. **SPIRAM dependency**: Both display double buffering and camera double buffering use SPIRAM. During concurrent operation, account for 1024x600x2 bytes/frame, camera frames, the total LVGL buffer size, and cache alignment.
11. **Wireless regulations**: SX1262 at 915 MHz/22 dBm is not permitted in all regions. In China, it generally must be reconfigured and certified according to the applicable frequency-band and transmit-power regulations.
12. **Non-reproducible configuration**: The repository does not include the final `sdkconfig` for each lesson. The source code alone cannot fully reproduce the target, PSRAM, MIPI PHY, wireless component switches, and other build settings.
13. **Schematic source issue**: The ESP32-P4 description attribute in the EAGLE `.sch` contains corrupted/garbled XML text. Although EAGLE/PDF and fault-tolerant parsing can still read the nets, this attribute should be repaired and ERC rerun before further editing.

## 17. Driver Porting Checklist

- [ ] Identify the target PCB revision and confirm the actual hardware conclusions for D-01 through D-08.
- [ ] Use ESP-IDF 5.4.x and pin the managed component versions.
- [ ] Create a unified `sdkconfig.defaults` that fixes the target, Flash, PSRAM, MIPI, LVGL, and wireless options.
- [ ] Consolidate all GPIO definitions into a single `board_pins.h`, retaining only one source of truth between Kconfig and the header file.
- [ ] After power-on, verify the 5 V, 3.3 V, 1.8 V, 2.8 V, and 1.2 V power rails before connecting the display/camera.
- [ ] Independently verify UART0 download, BOOT, RESET, and logging.
- [ ] Use an I2C scan/device IDs to verify GT911, DHT20 if externally connected, and camera SCCB.
- [ ] Keep the backlight disabled while first verifying successful DSI initialization, then gradually increase brightness.
- [ ] Start SD validation at 10 MHz and 1-bit; increase speed/bus width only after continuous read/write and hot-plug tests pass.
- [ ] Send silent audio frames first, verify GPIO30 polarity, and then enable the amplifiers.
- [ ] Test camera cold/warm starts, long-running double buffering, and dropped frames.
- [ ] Enable only one wireless module and verify the frequency band, antenna, peak power demand, and regulatory requirements.
- [ ] Complete a static GPIO allocation check and full-system peak memory/current budget.

## 18. Key Source Code Index

| Function | Main Files |
| --- | --- |
| Display/Backlight | `idf-code/Lesson07-Turn_on_the_screen/peripheral/bsp_illuminate/` |
| GT911/I2C | `idf-code/Lesson05-Touchscreen/peripheral/bsp_display/`, `bsp_i2c/` |
| USB HID | `idf-code/Lesson06-USB2.0/peripheral/bsp_usb/` |
| SDMMC/FAT | `idf-code/Lesson08-SD_Card_File_Reading/peripheral/bsp_sd/` |
| DHT20 | `idf-code/Lesson10-Temperature_and_Humidity/peripheral/bsp_dht20/` |
| PDM/I2S | `idf-code/Lesson11-Playback_After_Recording/peripheral/bsp_mic/`, `bsp_audio/` |
| SD music playback | `idf-code/Lesson12-Playing_Loca_Music_from_SD_Card/` |
| MIPI-CSI/SC2336 | `idf-code/Lesson13-Camera_Real-Time/peripheral/bsp_camera/` |
| SX1262 | `idf-code/Lesson14_TX_SX1262_Wireless_Module/` and the corresponding RX project |
| nRF24 | `idf-code/Lesson15_TX_nRF2401_Wireless_RF_Module/` and the corresponding RX project |
| Wi-Fi | `idf-code/Lesson16_Get_weather_via_WiFi/components/bsp_wifi/`, `Lesson17-Wi-Fi_function/` |
| Schematic/PCB | `1.2/ESP32-P4 Display 7.0 inch V1.2.sch/.pdf/.brd` |

## 19. Maintenance Recommendations

Going forward, use this document as input for a production-grade BSP instead of continuing to copy duplicate code from individual lessons. Establish a unified board component to centrally manage the pin map, power sequencing, and bus handles. With every hardware revision, update the schematic revision, `board_pins.h`, `sdkconfig.defaults`, and the discrepancy table in this document. Prioritize hardware validation of D-01, D-02/D-03, and D-08, as these are currently the three groups of issues most likely to block production porting.