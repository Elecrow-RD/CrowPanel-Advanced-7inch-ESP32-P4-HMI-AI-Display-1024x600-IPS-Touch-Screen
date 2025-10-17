### 1, Product picture



### 2, Product version number

|      | Hardware | Software | Remark |
| ---- | -------- | -------- | ------ |
| 1    | V1.0     | V1.0     | latest |

### 3, product information

| **Main Chip-ESP32-P4NRW32**                  |                                                              |
| -------------------------------------------- | ------------------------------------------------------------ |
| CPU/SoC                                      | **ESP32-P4**RISC-V 32-bit dual-core processor for HP systems, running at up to 400 MHz;RISC-V 32-bit single-core processor for LP systems, running at up to 40 MHz |
| System Memory                                | 768 KB L2MEM (HP) 32 KB SRAM (LP) 8 KB TCM 32 MB PSRAM       |
| Memory                                       | 128 KB ROM (HP) 16 KB ROM (LP) 16 MB Flash                   |
| Development Language                         | MicroPython, Rust, Lua                                       |
| Development Environment                      | ESP-IDF、Arduino IDE、LVGL                                   |
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

### 4, Use the driver module

| Name | dependency library |
| ---- | ------------------ |
| LVGL | lvgl/lvgl@8.3.11   |

### 5,Quick Start
##### Arduino IDE starts

1.Download the library files used by this product to the 'libraries' folder.

C:\Users\Documents\Arduino\libraries\

![2](https://github.com/user-attachments/assets/86c568bb-3921-4a07-ae91-62d7ce752e50)



2.Open the Arduino IDE

![1](https://github.com/user-attachments/assets/17b4e9af-a863-4bfd-839e-be94f00a33ad)


3.Open the code configuration environment and burn it

![3](https://github.com/user-attachments/assets/1a58d8ff-616b-4b71-9465-c2dac03f3399)



##### ESP-IDF starts

1.Right-click on an empty space in the project folder and select "Open with VS Code" to open the project.
![4](https://github.com/user-attachments/assets/a842ad62-ed8b-49c0-bfda-ee39102da467)



2.In the IDF plug-in, select the port, then compile and flash
![5](https://github.com/user-attachments/assets/76b6182f-0998-4496-920d-d262a5142df3)




### 6,Folder structure.
|--3D file： Contains 3D model files (.stp) for the hardware. These files can be used for visualization, enclosure design, or integration into CAD software.

|--Datasheet: Includes datasheets for components used in the project, providing detailed specifications, electrical characteristics, and pin configurations.

|--Eagle_SCH&PCB: Contains **Eagle CAD** schematic (`.sch`) and PCB layout (`.brd`) files. These are used for circuit design and PCB manufacturing.

|--example: Provides example code and projects to demonstrate how to use the hardware and libraries. These examples help users get started quickly.

|--factory_firmware: Stores pre-compiled factory firmware that can be directly flashed onto the device. This ensures the device runs the default functionality.

|--factory_sourcecode:  Contains the source code for the factory firmware, allowing users to modify and rebuild the firmware as needed.

|--libraries: Includes necessary libraries required for compiling and running the project. These libraries provide drivers and additional functionalities for the hardware.


### 7,Pin definition


