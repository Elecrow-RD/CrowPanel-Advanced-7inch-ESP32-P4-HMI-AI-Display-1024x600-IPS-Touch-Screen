### 1, Product picture

<img width="600" height="600" alt="image" src="https://github.com/user-attachments/assets/96031111-def0-44db-aa04-17fdfcc1354b" />



### 2, Product version number

|      | Hardware | Software | Remark |
| ---- | -------- | -------- | ------ |
| 1    | V1.0     | V1.0     | old |
| 1    | V1.1     | V1.1     | old |
| 1    | V1.2     | V1.2     | latest |

**Version 1.1** of the C6 module supports **4-wire SDIO** communication. The pin order for the SDIO pins has changed: **D0–D3** have been reassigned from **pins 14, 15, 16, and 17 to pins 17, 16, 15, and 14**.

**Version 1.2** improves the stability of the LoRa wireless module during communication.

### 3, product information

| **Main Chip-ESP32-P4NRW32**                  |                                                              |
| -------------------------------------------- | ------------------------------------------------------------ |
| CPU/SoC                                      | **ESP32-P4**RISC-V 32-bit dual-core processor for HP systems, running at up to 400 MHz;RISC-V 32-bit single-core processor for LP systems, running at up to 40 MHz |
| System Memory                                | 768 KB L2MEM (HP) 32 KB SRAM (LP) 8 KB TCM 32 MB PSRAM       |
| Memory                                       | 128 KB ROM (HP) 16 KB ROM (LP) 16 MB Flash                   |
| Development Environment                      | ESP-IDF、Arduino IDE                                   |
| **Screen**                                   |                                                              |
| Size                                         | 7.0 inch                                                     |
| Resolution                                   | 1024*600                                                     |
| Display Panel                                | IPS Panel                                                    |
| Touch Panel                                  | Capacitive Touch, Single/5-point Touch                       |
| Viewing Angle                                | 178°                                                         |
| Brightness                                   | 400 cd/m²(Typ.)                                              |
| Color Depth                                  | 16.7M (8-bit)                                                |
| **Wireless Communication - Onboard Antenna** |                                                              |
| WiFi                                         | Support 2.4GHz(Wi-Fi6), 802.11a/b/g/n                        |
| Bluetooth                                    | Support Bluetooth 5.3 and BLE                                |
| Other                                        | Zigbee, LoRa, nRF2401, Matter, Thread (**Optional**)         |
| **Interface/Function**                       |                                                              |
| Interface                                    | USB2.0, UART, I2C, GPIO female headers, SD card holder, battery socket, speaker jack, camera header, module female headers, etc. |
| Function                                     | Audio amplifier, battery charge management, USB to UART, dual microphones, dual speakers etc. |
| **Button/LED Indicator**                     |                                                              |
| Reset Button                                 | Yes, press to reset the device                               |
| Boot Button                                  | Yes, press and hold the power button to burn the program     |
| Power Button                                 | Switch On/Off                                                |
| PWR                                          | Device power on/off indication                               |
| CHG                                          | Lithium battery charging status, Low battery state           |
| **Other**                                    |                                                              |
| Installation method                          | All around mounting holes(M3 3.2mm), embedded, shell assembly |
| Operating temperature                        | -20~70 °C                                                    |
| Storage temperature                          | -30~80 °C                                                    |
| Power Input                                  | 5V/2A, USB or UART terminal                                  |
| Active Area                                  | 155mm*87mm                                                   |
| Dimensions                                   | 180*105mm                                                    |



### Functional description of the product's internal interfaces:

| Pin Name | Description                                                  | Connector Type |
| :------- | :----------------------------------------------------------- | :------------- |
| SPK      | Output audio signals to connect to speakers. The main board comes with a power amplifier chip circuit. | PH2.0-2P       |
| PWR      | Power LED.                                                   |                |
| RST      | Reset button. Press it to reset the system.                  |                |
| boot     |                                                              |                |
| UART1    | Builds communication between Logic modules, including the serial communication module and the print module. | HY2.0-4P       |
| I2C      | Builds communication between Logic modules, including the serial communication module and the print module. | HY2.0-4P       |
| UART3-IN | Input power supply and serial communication functionality    | XH2.54-4P      |
| BAT      | Connect the lithium battery. (with battery charging circuit) | PH2.0-2P       |





### 4, Use the driver module

| Name | dependency library |
| ---- | ------------------ |
| LVGL | lvgl/lvgl@9.2   |

### 5,Quick Start




##### ESP-IDF starts

1.Right-click on an empty space in the project folder and select "Open with VS Code" to open the project.
![4](https://github.com/user-attachments/assets/a842ad62-ed8b-49c0-bfda-ee39102da467)



2.In the IDF plug-in, select the port, then compile and flash

<img width="1363" height="721" alt="image" src="https://github.com/user-attachments/assets/784b78c0-65a8-4a3e-b98d-73f2f5d04e35" />



### 6,Folder structure.
|--3D file： Contains 3D model files (.stp) for the hardware. These files can be used for visualization, enclosure design, or integration into CAD software.

|--Datasheet: Includes datasheets for components used in the project, providing detailed specifications, electrical characteristics, and pin configurations.

|--Eagle_SCH&PCB: Contains **Eagle CAD** schematic (`.sch`) and PCB layout (`.brd`) files. These are used for circuit design and PCB manufacturing.

|--example: Provides example code and projects to demonstrate how to use the hardware and libraries. These examples help users get started quickly.

|--factory_firmware: Stores pre-compiled factory firmware that can be directly flashed onto the device. This ensures the device runs the default functionality.

|--factory_sourcecode:  Contains the source code for the factory firmware, allowing users to modify and rebuild the firmware as needed.

|--libraries: Includes necessary libraries required for compiling and running the project. These libraries provide drivers and additional functionalities for the hardware.


### 7,Pin definition

#### ESP32-P4 7 inch and IPS Display Wiring Pins:
**DSI = Display Serial Interface**is a high-speed, low-power display interface standard defined by the MIPI Alliance, most commonly used in smartphones, tablets, Raspberry Pi devices, and embedded Linux systems.

<img width="723" height="693" alt="image" src="https://github.com/user-attachments/assets/f74e408c-d66c-44f9-a88b-de0172dc0e28" />


DSI Pin connection

DSI_DATAN0--IO40

DSI_DATAN0--IO39

DSI_DATAN0--IO36

DSI_DATAN0--IO35

DSI_CLKN--IO37

DSI_CLKP--IO38

DSI_REXT--IO34

#### ESP32-P4 and Touch Driver Wiring：

i2c address: 0x5D/0x14.(The INT pin level during reset of the GT911 touch chip determines the device address.)

INT Low Level(0x5D);

INT High Level(0x14).

<img width="641" height="571" alt="image" src="https://github.com/user-attachments/assets/f5a27de9-679b-4199-b611-78e549d06f64" />


Pin connection

I2C1_SCL(IO46)

I2C1_SDA(IO45)

INT_TP(IO42)

RESET_TP(IO40)

#### ESP32-P4 and wireless module wiring pins：

Output voltage: 3.3V Output current: 1A max. Use: The power supply communicates with the wireless module.

<img width="1008" height="553" alt="image" src="https://github.com/user-attachments/assets/afea7188-35c8-4ef2-92ff-d0387e378274" />


Pin connection


#define RADIO_GPIO_CLK 8

#define RADIO_GPIO_MISO 7

#define RADIO_GPIO_MOSI 6

#ifdef CONFIG_BSP_SX1262_ENABLED

#define SX1262_GPIO_BUSY 9

#define SX1262_GPIO_IRQ 53

#define SX1262_GPIO_NRST 54

#define SX1262_GPIO_NSS 10

#ifdef CONFIG_BSP_NRF2401_ENABLED

#define NRF24_GPIO_IRQ 9

#define NRF24_GPIO_CE 53

#define NRF24_GPIO_CS 54

ESP32-P4 and Audio out：

<img width="913" height="789" alt="image" src="https://github.com/user-attachments/assets/7f9e0056-6d34-4d90-a899-7a642a0af6e1" />


Pin connection


#define AUDIO_GPIO_LRCLK    21   // GPIO pin number for LRCLK (Left-Right Clock)

#define AUDIO_GPIO_BCLK     22   // GPIO pin number for BCLK (Bit Clock)

#define AUDIO_GPIO_SDATA    23   // GPIO pin number for SDATA (Serial Data)

#define AUDIO_GPIO_CTRL     30   // GPIO pin number for audio amplifier control

