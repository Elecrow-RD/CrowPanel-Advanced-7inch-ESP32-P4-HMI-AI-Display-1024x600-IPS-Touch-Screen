/*---------------------------------------------------------------
 * Header file declarations
 *--------------------------------------------------------------*/
#include "bsp_usb.h"

static const char *TAG = "USB_HID";

/*---------------------------------------------------------------
 * TinyUSB descriptors
 *--------------------------------------------------------------*/

/* Total length of the configuration descriptor: config + one HID interface. */
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor.
 *
 * Declares a single mouse report so the host enumerates the device
 * as a mouse using the mouse boot protocol.
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

/**
 * @brief USB string descriptors.
 *
 * Index 0 is the language ID (English), the rest are manufacturer,
 * product, serial and interface strings shown by the host OS.
 */
const char *hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},   /* 0: English (0x0409). */
    "Espressif",             /* 1: Manufacturer. */
    "Advance-P4 HID Mouse",  /* 2: Product name. */
    "123456",                 /* 3: Serial number. */
    "HID Mouse Interface",   /* 4: Interface string. */
};

/**
 * @brief Configuration descriptor containing one HID interface.
 *
 * Power is set to 100 mA and remote wakeup is advertised.
 */
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    /* Endpoint 0x81 IN, 16 bytes, 10 ms polling interval. */
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/*---------------------------------------------------------------
 * TinyUSB HID callbacks
 *--------------------------------------------------------------*/

/**
 * @brief Return the HID report descriptor when the host requests it.
 */
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

/**
 * @brief Handle GET_REPORT control requests.
 *
 * Not used by the boot mouse protocol; returns 0 to stall the request.
 */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

/**
 * @brief Handle SET_REPORT control requests.
 *
 * Not used by this example; left empty.
 */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

/*---------------------------------------------------------------
 * Application API
 *--------------------------------------------------------------*/

/**
 * @brief Send a relative mouse movement to the host.
 *
 * @param delta_x Horizontal movement (signed, mouse units).
 * @param delta_y Vertical movement (signed, mouse units).
 */
void send_hid_mouse_delta(int8_t delta_x, int8_t delta_y)
{
    send_hid_mouse_report(0, delta_x, delta_y);
}

void send_hid_mouse_report(uint8_t buttons, int8_t delta_x, int8_t delta_y)
{
    /* Only send when the host has finished enumeration and is ready. */
    if (tud_hid_ready()) {
        /* Buttons=0, no wheel, no pan. */
        /* Match the report ID declared by TUD_HID_REPORT_DESC_MOUSE(). */
        tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, buttons,
                             delta_x, delta_y, 0, 0);
    }
}

/**
 * @brief Report whether the HID interface is ready to send reports.
 * @return true if the host is ready.
 */
bool is_usb_ready(void)
{
    return tud_hid_ready();
}

/**
 * @brief Install the TinyUSB driver with the HID mouse configuration.
 *
 * @return ESP_OK on success.
 */
esp_err_t usb_init(void)
{
    ESP_LOGI(TAG, "Initializing USB HID Mouse");

    /* esp_tinyusb 2.x groups PHY, task and descriptors in nested fields. */
    /* ESP32-P4 exposes the USB 2.0 OTG controller on the high-speed port. */
    tinyusb_config_t tusb_cfg = TINYUSB_CONFIG_HIGH_SPEED(NULL, NULL);
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.string_count = sizeof(hid_string_descriptor) /
                                       sizeof(hid_string_descriptor[0]);
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "USB HID Mouse initialization completed");
    return ESP_OK;
}
