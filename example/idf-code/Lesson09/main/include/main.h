#ifndef _MAIN_H_
#define _MAIN_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ldo_regulator.h"  // Include LDO (Low Dropout Regulator) API
#include "esp_timer.h"
#include "lvgl.h"
#include "bsp_extra.h"
#include "bsp_display.h"
#include "bsp_illuminate.h"
#include "bsp_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

/* Log macro definition */
#define MAIN_TAG "MAIN"
#define MAIN_INFO(fmt, ...) ESP_LOGI(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_DEBUG(fmt, ...) ESP_LOGD(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_ERROR(fmt, ...) ESP_LOGE(MAIN_TAG, fmt, ##__VA_ARGS__)

/*—————————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif