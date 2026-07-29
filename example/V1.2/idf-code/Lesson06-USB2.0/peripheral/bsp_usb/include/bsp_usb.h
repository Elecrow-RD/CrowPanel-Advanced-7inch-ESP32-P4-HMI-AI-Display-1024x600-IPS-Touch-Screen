#ifndef _BSP_USB_H_
#define _BSP_USB_H_

#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinyusb.h"
#include "class/hid/hid_device.h"

/*---------------------------------------------------------------
 * Logging macros
 *--------------------------------------------------------------*/
#define USB_TAG "USB"

#define USB_INFO(fmt, ...)   ESP_LOGI(USB_TAG, fmt, ##__VA_ARGS__)
#define USB_DEBUG(fmt, ...)  ESP_LOGD(USB_TAG, fmt, ##__VA_ARGS__)
#define USB_ERROR(fmt, ...)  ESP_LOGE(USB_TAG, fmt, ##__VA_ARGS__)

/* HID boot protocol value that identifies a mouse interface. */
#define HID_ITF_PROTOCOL_MOUSE 1

/*---------------------------------------------------------------
 * Public API
 *--------------------------------------------------------------*/

/**
 * @brief Send a relative mouse movement as an HID report.
 * @param delta_x Horizontal delta (mouse units).
 * @param delta_y Vertical delta (mouse units).
 */
void send_hid_mouse_delta(int8_t delta_x, int8_t delta_y);

/**
 * @brief Return whether the HID device is ready to send reports.
 */
bool is_usb_ready(void);

/**
 * @brief Initialise the TinyUSB HID mouse device.
 * @return ESP_OK on success.
 */
esp_err_t usb_init(void);

#endif
