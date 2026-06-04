#ifndef __BSP_WIFI_H_
#define __BSP_WIFI_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"

#include "esp_log.h"
#include "nvs_flash.h"

typedef enum {
    WIFI_RUN_MODE_IDLE,
    WIFI_RUN_MODE_STA,
    WIFI_RUN_MODE_AP,
    WIFI_RUN_MODE_IS_DEINIT,
} EM_WIFI_MODE_STATE;

typedef enum {
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
} EM_WIFI_STATE;

#define AP_SSID         "ESP32-P4-WIFI"
#define AP_PASSWORD     "12345678"
#define AP_CHANNEL      6
#define AP_MAX_CON      4

void bsp_wifi_init(void);
void bsp_wifi_ap_sta_deinit(void);

void bsp_wifi_ap_init(void);

void bsp_wifi_sta_init(void);
void bsp_wifi_connect(char* ssid, char* password);
void bsp_wifi_disconnect(void);
EM_WIFI_STATE bsp_wifi_get_state(void);

#endif //__BSP_WIFI_H_
