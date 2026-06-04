#ifndef _BSP_USB_H_          // Prevent multiple inclusion of this header file
#define _BSP_USB_H_

#include <stdint.h>           // Standard integer types
#include "esp_err.h"          // ESP-IDF error handling definitions
#include "esp_log.h"          // ESP-IDF logging functions
#include "freertos/FreeRTOS.h" // FreeRTOS base header
#include "freertos/task.h"    // FreeRTOS task management APIs

#include "tinyusb.h"          // TinyUSB core library
#include "class/hid/hid_device.h" // TinyUSB HID device class definitions

#define USB_TAG "USB"         // Logging tag for USB module
#define USB_INFO(fmt, ...) ESP_LOGI(USB_TAG, fmt, ##__VA_ARGS__)  // Info log macro
#define USB_DEBUG(fmt, ...) ESP_LOGD(USB_TAG, fmt, ##__VA_ARGS__) // Debug log macro
#define USB_ERROR(fmt, ...) ESP_LOGE(USB_TAG, fmt, ##__VA_ARGS__) // Error log macro

// HID protocol definition
#define HID_ITF_PROTOCOL_MOUSE   1  // HID interface protocol: Mouse

// Function to send mouse movement deltas over USB HID
void send_hid_mouse_delta(int8_t delta_x, int8_t delta_y);

// Function to check whether USB is initialized and ready
bool is_usb_ready(void);

// Function to initialize USB subsystem
esp_err_t usb_init(void);

#endif // _BSP_USB_H_
