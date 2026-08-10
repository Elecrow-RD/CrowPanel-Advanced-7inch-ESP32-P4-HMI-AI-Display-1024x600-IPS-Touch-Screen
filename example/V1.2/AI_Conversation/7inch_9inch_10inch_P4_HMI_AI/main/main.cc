#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "application.h"
#include "system_info.h"

#define TAG "main"

/*---------------------------------------------------------------
 * Application entry point
 * Prepare persistent storage and networking before any board or
 * application object attempts to use those shared ESP-IDF services.
 *--------------------------------------------------------------*/

/**
 * @brief Initialize platform services and start the application event loop.
 *
 * NVS is repaired when its stored layout no longer matches the running
 * firmware. The network interface and default event loop are created before
 * waiting for the ESP32-C6 Wi-Fi coprocessor and starting the application.
 *
 * @param None.
 * @return None; this ESP-IDF entry point remains in MainEventLoop().
 * @note Called once by ESP-IDF after the scheduler and runtime are ready.
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Reset reason: %d", static_cast<int>(esp_reset_reason()));

    // Network credentials and device settings depend on NVS being available.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Wi-Fi drivers require both esp_netif and the default event loop.
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ESP-Hosted needs the C6 and its SDIO link ready before Wi-Fi starts.
    ESP_LOGI(TAG, "Waiting for ESP32-C6 coprocessor initialization...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    auto& app = Application::GetInstance();
    app.Start();
    app.MainEventLoop();
}
