#ifndef _BSP_CAMERA_H_
#define _BSP_CAMERA_H_
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/isp.h"
#include "esp_etm.h"
#include "esp_async_memcpy.h"
#include "esp_sccb_intf.h"
#include "esp_sccb_i2c.h"
#include "esp_cam_sensor.h"
#include "esp_cam_sensor_detect.h"
#include "esp_cam_ctlr_csi.h"
#include "esp_cam_ctlr.h"
#include "esp_cache.h"
#include "hal/cache_ll.h"
#include "hal/cache_hal.h"
#include "bsp_illuminate.h"

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define CAMERA_TAG "CAMERA"
#define CAMERA_INFO(fmt, ...) ESP_LOGI(CAMERA_TAG, fmt, ##__VA_ARGS__)
#define CAMERA_DEBUG(fmt, ...) ESP_LOGD(CAMERA_TAG, fmt, ##__VA_ARGS__)
#define CAMERA_ERROR(fmt, ...) ESP_LOGE(CAMERA_TAG, fmt, ##__VA_ARGS__)

#define SCCB_MASTER_PORT 1
#define SCCB_GPIO_SCL 13
#define SCCB_GPIO_SDA 12

esp_err_t camera_init();
void camera_display();
esp_err_t camera_refresh();
void camera_display_refresh();

extern lv_img_dsc_t img_camera;
extern esp_cam_ctlr_trans_t my_trans;
extern esp_cam_ctlr_handle_t cam_handle;

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
