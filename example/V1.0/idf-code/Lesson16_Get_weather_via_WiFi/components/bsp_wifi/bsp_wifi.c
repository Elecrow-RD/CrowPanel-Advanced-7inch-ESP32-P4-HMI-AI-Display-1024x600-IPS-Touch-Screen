/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "bsp_wifi.h"

static const char *TAG = "bsp_wifi";

EM_WIFI_MODE_STATE wifi_run_state_ = WIFI_RUN_MODE_IDLE;
EM_WIFI_STATE wifi_sta_state_ = WIFI_IDLE;
char wifi_ssid_[64] = "";
char wifi_password_[64] = "";
esp_netif_t *netif_ = NULL;  // Network interface object

void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{   
    if(event_base == WIFI_EVENT) {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:      // Triggered after WiFi starts in STA mode
            // esp_wifi_connect();         // Start WiFi connection
            // instance->wifi_sta_state_ = WIFI_CONNECTING;
            break;
        case WIFI_EVENT_STA_CONNECTED:  // Triggered after WiFi connects to the router
            wifi_ap_record_t ap_info;
            esp_wifi_sta_get_ap_info(&ap_info); // Get current AP information
            printf("connected to AP successfully\n");
            printf("Connected RSSI: %d dBm\n", ap_info.rssi);
            break;
        case WIFI_EVENT_STA_DISCONNECTED:   // Triggered after WiFi disconnects from the router
            if (WIFI_IDLE != wifi_sta_state_) {
                esp_wifi_connect();             // Retry connection
                wifi_sta_state_ = WIFI_DISCONNECTED;
                printf("connect to the AP fail,retry now\n");
            }
            break;


        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t* event1=(wifi_event_ap_staconnected_t*) event_data;
            printf("station " MACSTR " join,AID=%d\n", MAC2STR(event1->mac), event1->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t* event2=(wifi_event_ap_stadisconnected_t*) event_data;
            printf( "station " MACSTR " leave,AID=%d,reason=%d\n", MAC2STR(event2->mac),event2->aid,event2->reason);
            break;
        }
        default:
            break;
        }
    }

    else if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            // Only when the IP address assigned by the router is obtained is it considered to be connected to the network
            case IP_EVENT_STA_GOT_IP:
                wifi_sta_state_ = WIFI_CONNECTED;
                ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
                printf("WiFi connected with IP:" IPSTR "\n", IP2STR(&event->ip_info.ip));
                break;
        }
    }
}

void bsp_wifi_connect(char* ssid, char* password)
{
    esp_wifi_stop();
    // WiFi configuration
    wifi_config_t wifi_config = 
    { 
        .sta = 
        { 
            .pmf_cfg = 
            {
                .capable = true,
                .required = false
            },
        },
    };

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,   // Encryption mode
    strncpy((char*)wifi_config.sta.ssid, ssid, 32);        // WiFi SSID
    strncpy((char*)wifi_config.sta.password, password, 64);    // WiFi password

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );   // Set WiFi configuration
    esp_wifi_start();
    esp_wifi_connect();
    wifi_sta_state_ = WIFI_CONNECTING;
}

void bsp_wifi_disconnect(void)
{
    esp_wifi_disconnect();
    esp_wifi_stop();
}

void bsp_wifi_init(void)
{
    ESP_LOGI(TAG, "bsp_wifi_init");
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize base network stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void bsp_wifi_sta_init(void)
{
    ESP_LOGI(TAG, "bsp_wifi_sta_init");
    if (NULL != netif_) {
        return;
    }

    netif_ = esp_netif_create_default_wifi_sta(); // Create STA object using default configuration

    // Register events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Initialize default STA configuration
    wifi_config_t wifi_config = {0};

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );         // Set working mode to STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );   // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));    // Store WiFi configuration in RAM
    ESP_ERROR_CHECK(esp_wifi_start());                         // Start WiFi

    wifi_run_state_ = WIFI_RUN_MODE_STA;
}

void bsp_wifi_ap_init(void)
{
    ESP_LOGI(TAG, "bsp_wifi_ap_init");
    if (NULL != netif_) {
        return;
    }
    
    netif_ = esp_netif_create_default_wifi_ap(); // Create default AP network interface

    // Register events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // 3. Configure AP parameters
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,                         // Channel (1-13)
            .authmode = WIFI_AUTH_WPA2_PSK,       // Encryption mode
            .max_connection = AP_MAX_CON,
        },
    };
    // if (strlen(AP_PASSWORD) < 8) {
    //     wifi_config.ap.authmode = WIFI_AUTH_OPEN;  // Disable encryption if password is empty
    // }

    // 4. Start AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));    // Store WiFi configuration in RAM
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_run_state_ = WIFI_RUN_MODE_AP;

    printf("WIFI SSID = %s\n", AP_SSID);
    printf("WIFI password = %s\n", AP_PASSWORD);
}

EM_WIFI_STATE bsp_wifi_get_state(void)
{
    return wifi_sta_state_;
}

void bsp_wifi_ap_sta_deinit(void)
{
    ESP_LOGI(TAG, "bsp_wifi_ap_sta_deinit");
    if (NULL == netif_) {
        return;
    }
    wifi_run_state_ = WIFI_RUN_MODE_IS_DEINIT;

    // Stop WiFi and disconnect
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for WiFi to fully stop
    
    // Unregister WiFi event handlers
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    
    // // Deinitialize WiFi driver
    // esp_wifi_deinit();

    // Delete default network interface
    if (netif_) {
        esp_netif_destroy(netif_);
        netif_ = NULL;
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for driver to fully release
    wifi_run_state_ = WIFI_RUN_MODE_IDLE;
}
