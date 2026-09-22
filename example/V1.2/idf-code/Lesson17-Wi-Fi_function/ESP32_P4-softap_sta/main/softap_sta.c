/*---------------------------------------------------------------
 * WiFi softAP + Station (APSTA) example with NAPT
 *
 * Target board: ESP32-P4 + ESP32-C6 (ESP-Hosted slave over SDIO).
 * ---------------------------------------------------------------
 * The ESP32-P4 has NO Wi-Fi radio of its own. Every esp_wifi_xxx()
 * call made here is forwarded over SDIO to the ESP32-C6 by the
 * esp_wifi_remote + esp_hosted components; the C6 owns the radio
 * and runs both the softAP and the station.
 *
 * Behaviour of this application:
 *   - The softAP is always kept alive (default SSID "ELECROW").
 *   - NAPT is enabled on the AP netif as soon as the AP is up, so
 *     clients get Internet as soon as the station has an IP.
 *   - The station keeps trying to join the upstream router forever
 *     (see s_sta_reconnect_task); it never blocks the AP.
 *   - The upstream DNS server is pushed to the AP's DHCP server
 *     each time the station obtains an IP.
 *--------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif

/*---------------------------------------------------------------
 * Configuration (menuconfig -> Example Configuration)
 *--------------------------------------------------------------*/

/* STA credentials - the upstream AP (router) to join. */
#define EXAMPLE_ESP_WIFI_STA_SSID           CONFIG_ESP_WIFI_REMOTE_AP_SSID
#define EXAMPLE_ESP_WIFI_STA_PASSWD         CONFIG_ESP_WIFI_REMOTE_AP_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY           CONFIG_ESP_MAXIMUM_STA_RETRY

/* Auth mode threshold used during STA scanning. */
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WAPI_PSK
#endif

/* AP credentials - the hotspot this device advertises. */
#define EXAMPLE_ESP_WIFI_AP_SSID            CONFIG_ESP_WIFI_AP_SSID
#define EXAMPLE_ESP_WIFI_AP_PASSWD          CONFIG_ESP_WIFI_AP_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL            CONFIG_ESP_WIFI_AP_CHANNEL
#define EXAMPLE_MAX_STA_CONN                CONFIG_ESP_MAX_STA_CONN_AP

/* Event group bits. */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* DHCP server option: advertise the DNS server to clients. */
#define DHCPS_OFFER_DNS 0x02

/* Delay before an automatic STA reconnection attempt. */
#define EXAMPLE_STA_RECONNECT_DELAY_MS      3000

/* How long app_main waits for the first successful STA connection. */
#define EXAMPLE_STA_FIRST_CONNECT_TIMEOUT_MS 60000

static const char *TAG_AP  = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Sta";

static int s_retry_num = 0;
static volatile bool s_sta_connected = false;
static volatile bool s_sta_fail_reported = false;

/* Event group signalling connection success / failure. */
static EventGroupHandle_t s_wifi_event_group;

/* Netif handles (also used from the event handler). */
static esp_netif_t *s_netif_ap  = NULL;
static esp_netif_t *s_netif_sta = NULL;

/* Handles the delayed reconnection to the upstream AP. */
static TaskHandle_t s_sta_reconnect_task = NULL;

/* True once NAPT has been successfully enabled on the AP netif. */
static bool s_napt_enabled = false;

/*---------------------------------------------------------------
 * Logging helpers
 *--------------------------------------------------------------*/

/**
 * @brief Log the softAP SSID / password / channel and its IP address.
 *
 * Printing the AP IP unconditionally means the serial log always
 * shows a usable address, even when the station cannot join the
 * upstream router.
 */
static void log_ap_info(void)
{
    if (s_netif_ap == NULL) {
        return;
    }

    esp_netif_ip_info_t ip = { 0 };
    esp_err_t err = esp_netif_get_ip_info(s_netif_ap, &ip);

    ESP_LOGI(TAG_AP, "SoftAP \"%s\" password:\"%s\" channel:%d, connect your phone to it",
             EXAMPLE_ESP_WIFI_AP_SSID, EXAMPLE_ESP_WIFI_AP_PASSWD, EXAMPLE_ESP_WIFI_CHANNEL);

    if (err == ESP_OK) {
        ESP_LOGI(TAG_AP, "SoftAP IP:" IPSTR ", gateway:" IPSTR ", mask:" IPSTR,
                 IP2STR(&ip.ip), IP2STR(&ip.gw), IP2STR(&ip.netmask));
    } else {
        ESP_LOGW(TAG_AP, "SoftAP has no IP yet (%s)", esp_err_to_name(err));
    }
}

/**
 * @brief Log the IP address currently assigned to the STA netif.
 * @return true when the STA owns a usable IPv4 address.
 */
static bool log_sta_info(void)
{
    if (s_netif_sta == NULL) {
        return false;
    }

    esp_netif_ip_info_t ip = { 0 };
    if (esp_netif_get_ip_info(s_netif_sta, &ip) != ESP_OK || ip.ip.addr == 0) {
        ESP_LOGW(TAG_STA, "Station has no IP yet - not joined \"%s\"",
                 EXAMPLE_ESP_WIFI_STA_SSID);
        return false;
    }

    ESP_LOGI(TAG_STA, "Station IP:" IPSTR ", gateway:" IPSTR,
             IP2STR(&ip.ip), IP2STR(&ip.gw));
    return true;
}

/*---------------------------------------------------------------
 * NAPT / DNS plumbing
 *--------------------------------------------------------------*/

/**
 * @brief Enable NAPT on the AP netif (idempotent).
 *
 * NAPT is what lets clients behind the softAP reach the Internet
 * through the station interface. It only needs the AP netif to be
 * up, so it is enabled as soon as WIFI_EVENT_AP_START arrives and
 * therefore no longer depends on the station being connected.
 */
static void napt_enable_once(void)
{
    if (s_napt_enabled || s_netif_ap == NULL) {
        return;
    }

    esp_err_t err = esp_netif_napt_enable(s_netif_ap);
    if (err == ESP_OK) {
        s_napt_enabled = true;
        ESP_LOGI(TAG_AP, "NAPT enabled on the softAP interface");
    } else {
        ESP_LOGW(TAG_AP, "NAPT not enabled yet (%s), retrying on next STA event",
                 esp_err_to_name(err));
    }
}

/**
 * @brief Forward the STA's DNS server to the AP's DHCP clients.
 *
 * Stops the AP DHCP, sets the DNS offer option, copies the STA DNS
 * (or, when the upstream did not provide one, the upstream gateway)
 * into the AP, then restarts DHCP so clients behind the AP can
 * resolve names through the upstream router.
 *
 * Called every time the station obtains an IP, including after a
 * reconnection.
 */
static void softap_set_dns_addr(void)
{
    if (s_netif_ap == NULL || s_netif_sta == NULL) {
        return;
    }

    esp_netif_dns_info_t dns = { 0 };
    esp_netif_get_dns_info(s_netif_sta, ESP_NETIF_DNS_MAIN, &dns);

    if (dns.ip.u_addr.ip4.addr == 0) {
        /* Some routers only hand out an address; use the gateway as DNS. */
        esp_netif_ip_info_t sta_ip = { 0 };
        if (esp_netif_get_ip_info(s_netif_sta, &sta_ip) == ESP_OK) {
            dns.ip.u_addr.ip4.addr = sta_ip.gw.addr;
        }
        dns.ip.type = ESP_IPADDR_TYPE_V4;
    }

    if (dns.ip.u_addr.ip4.addr == 0) {
        ESP_LOGW(TAG_AP, "No DNS server learnt from the upstream AP, "
                 "clients may fail to resolve domain names");
        return;
    }

    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(s_netif_ap));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_option(s_netif_ap, ESP_NETIF_OP_SET,
                                    ESP_NETIF_DOMAIN_NAME_SERVER,
                                    &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_dns_info(s_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(s_netif_ap));

    ESP_LOGI(TAG_AP, "AP DHCP clients now use DNS " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
}

/*---------------------------------------------------------------
 * Delayed STA reconnection
 *--------------------------------------------------------------*/

/**
 * @brief Reconnect the station after a disconnect.
 *
 * Runs in its own task so the default event loop is never blocked.
 * The AP (and NAPT) keeps running while the upstream router is
 * unavailable, and the station joins it as soon as it is back.
 */
static void sta_reconnect_task(void *arg)
{
    (void)arg;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(EXAMPLE_STA_RECONNECT_DELAY_MS));
        if (!s_sta_connected) {
            ESP_LOGI(TAG_STA, "Reconnecting to \"%s\" ...", EXAMPLE_ESP_WIFI_STA_SSID);
            esp_wifi_connect();
        }
    }
}

/*---------------------------------------------------------------
 * WiFi event handler
 *--------------------------------------------------------------*/

/**
 * @brief Handle AP and STA events in APSTA mode.
 *
 * Logs station join/leave on the AP side; triggers connect, retries
 * and signals connected/failed on the STA side.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_AP_START:
            log_ap_info();
            /* The AP netif is up now, NAPT can be armed. */
            napt_enable_once();
            break;

        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG_AP, "Station " MACSTR " left, AID=%d, reason:%d",
                     MAC2STR(event->mac), event->aid, event->reason);
            break;
        }

        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG_STA, "Station started, connecting to \"%s\" ...",
                     EXAMPLE_ESP_WIFI_STA_SSID);
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
            s_sta_connected = false;
            ESP_LOGW(TAG_STA, "Disconnected from \"%s\", reason:%d (retry %d/%d)",
                     EXAMPLE_ESP_WIFI_STA_SSID, event->reason,
                     s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);

            s_retry_num++;
            if (s_retry_num >= EXAMPLE_ESP_MAXIMUM_RETRY && !s_sta_fail_reported) {
                /* Report the failure once so app_main can log a summary.
                 * Reconnection attempts continue in the background. */
                s_sta_fail_reported = true;
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            if (s_sta_reconnect_task != NULL) {
                xTaskNotifyGive(s_sta_reconnect_task);
            }
            break;
        }

        default:
            break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG_STA, "connected to ap SSID:%s", EXAMPLE_ESP_WIFI_STA_SSID);

            s_retry_num = 0;
            s_sta_fail_reported = false;
            s_sta_connected = true;
            xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

            softap_set_dns_addr();
            napt_enable_once();
        } else if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
            ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
            ESP_LOGI(TAG_AP, "Client " MACSTR " got IP:" IPSTR,
                     MAC2STR(event->mac), IP2STR(&event->ip));
            /* The AP netif is up for sure now: arm NAPT if AP_START raced. */
            napt_enable_once();
        }
    }
}

/*---------------------------------------------------------------
 * AP and STA init
 *--------------------------------------------------------------*/

/**
 * @brief Create the AP netif and configure the AP SSID/password.
 * @return The AP netif handle.
 */
esp_netif_t *wifi_init_softap(void)
{
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_AP_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_AP_PASSWD,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    /* No password -> open AP. */
    if (strlen(EXAMPLE_ESP_WIFI_AP_PASSWD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_AP_SSID, EXAMPLE_ESP_WIFI_AP_PASSWD, EXAMPLE_ESP_WIFI_CHANNEL);

    return esp_netif_ap;
}

/**
 * @brief Create the STA netif and configure the STA SSID/password.
 * @return The STA netif handle.
 */
esp_netif_t *wifi_init_sta(void)
{
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_STA_SSID,
            .password = EXAMPLE_ESP_WIFI_STA_PASSWD,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    ESP_LOGI(TAG_STA, "wifi_init_sta finished. Target SSID:%s", EXAMPLE_ESP_WIFI_STA_SSID);

    return esp_netif_sta;
}

/*---------------------------------------------------------------
 * Application entry point
 *--------------------------------------------------------------*/

/**
 * @brief Start WiFi in APSTA mode and enable NAPT forwarding.
 *
 * Brings up both the AP and STA interfaces, connects the STA to the
 * upstream router (with automatic retries), then enables Network
 * Address Port Translation so clients connected to the AP can reach
 * the internet through the STA.
 */
void app_main(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();
    xTaskCreate(sta_reconnect_task, "sta_reconnect", 3072, NULL, 5, &s_sta_reconnect_task);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* APSTA mode runs both interfaces simultaneously. */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
    s_netif_ap = wifi_init_softap();

    ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
    s_netif_sta = wifi_init_sta();

    ESP_ERROR_CHECK(esp_wifi_start());

    /* The AP must work even if the station never joins the router. */
    log_ap_info();

    /* Wait (bounded) for the first successful STA connection. */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(EXAMPLE_STA_FIRST_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_STA, "connected to ap SSID:%s", EXAMPLE_ESP_WIFI_STA_SSID);
        /* STA is the upstream interface; AP clients forward through it. */
        esp_netif_set_default_netif(s_netif_sta);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG_STA, "Failed to connect to SSID:\"%s\". Check the SSID/password in "
                 "menuconfig -> Example Configuration -> WiFi Remote AP SSID/Password "
                 "(the upstream router must be a 2.4 GHz network). "
                 "The softAP stays up and reconnection keeps retrying.",
                 EXAMPLE_ESP_WIFI_STA_SSID);
    } else {
        ESP_LOGW(TAG_STA, "Still not connected to \"%s\" after %d s; "
                 "the softAP keeps running and reconnection continues in the background",
                 EXAMPLE_ESP_WIFI_STA_SSID, EXAMPLE_STA_FIRST_CONNECT_TIMEOUT_MS / 1000);
    }

    /* Make sure NAPT is armed even if AP_START raced with this task. */
    napt_enable_once();

    /* Final state summary. */
    ESP_LOGI(TAG_AP, "=== softAP ready: SSID:\"%s\" password:\"%s\" ===",
             EXAMPLE_ESP_WIFI_AP_SSID, EXAMPLE_ESP_WIFI_AP_PASSWD);
    log_ap_info();
    bool sta_up = log_sta_info();
    ESP_LOGI(TAG_STA, "=== STA %s, NAPT %s ===",
             sta_up ? "online" : "offline",
             s_napt_enabled ? "enabled" : "NOT enabled");
}
