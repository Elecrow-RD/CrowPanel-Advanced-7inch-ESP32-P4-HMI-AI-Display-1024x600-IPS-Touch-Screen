/*
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <limits.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

#undef ESP_UTILS_LOG_TAG
#define ESP_UTILS_LOG_TAG "LvPort"
#include "esp_lib_utils.h"
#include "lvgl_port.h"

using namespace esp_panel::drivers;

static SemaphoreHandle_t lvgl_mux = nullptr;
static SemaphoreHandle_t touch_detected = nullptr;
static TaskHandle_t lvgl_task_handle = nullptr;
static esp_timer_handle_t lvgl_tick_timer = nullptr;
static lv_display_t *lvgl_display = nullptr;
static lv_indev_t *lvgl_indev = nullptr;
static void *lvgl_buf[2] = {};

#if LVGL_PORT_AVOID_TEAR
static void flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    LCD *lcd = static_cast<LCD *>(lv_display_get_user_data(disp));

    if (lv_display_flush_is_last(disp)) {
        ulTaskNotifyValueClear(nullptr, ULONG_MAX);
        lcd->switchFrameBufferTo(px_map);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    lv_display_flush_ready(disp);
}

IRAM_ATTR static bool on_lcd_vsync_callback(void *user_data)
{
    BaseType_t need_yield = pdFALSE;
    xTaskNotifyFromISR(static_cast<TaskHandle_t>(user_data), ULONG_MAX, eNoAction, &need_yield);
    return need_yield == pdTRUE;
}
#else
static void flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    LCD *lcd = static_cast<LCD *>(lv_display_get_user_data(disp));
    lcd->drawBitmap(
        area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1,
        static_cast<const uint8_t *>(px_map)
    );

    if (lcd->getBus()->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        lv_display_flush_ready(disp);
    }
}

IRAM_ATTR static bool on_draw_bitmap_finish_callback(void *user_data)
{
    lv_display_flush_ready(static_cast<lv_display_t *>(user_data));
    return false;
}
#endif

static lv_display_t *display_init(LCD *lcd)
{
    ESP_UTILS_CHECK_FALSE_RETURN(lcd != nullptr, nullptr, "Invalid LCD device");
    ESP_UTILS_CHECK_FALSE_RETURN(lcd->getRefreshPanelHandle() != nullptr, nullptr, "LCD device is not initialized");

    const int32_t lcd_width = lcd->getFrameWidth();
    const int32_t lcd_height = lcd->getFrameHeight();
    const uint32_t bytes_per_pixel = lv_color_format_get_size(LV_COLOR_FORMAT_RGB565);
    uint32_t buffer_size_bytes = 0;

#if LVGL_PORT_AVOID_TEAR
    buffer_size_bytes = static_cast<uint32_t>(lcd_width) * lcd_height * bytes_per_pixel;
    lvgl_buf[0] = lcd->getFrameBufferByIndex(0);
    lvgl_buf[1] = lcd->getFrameBufferByIndex(1);
#else
    buffer_size_bytes = static_cast<uint32_t>(lcd_width) * LVGL_PORT_BUFFER_SIZE_HEIGHT * bytes_per_pixel;
    for (int i = 0; i < LVGL_PORT_BUFFER_NUM; ++i) {
        lvgl_buf[i] = heap_caps_malloc(buffer_size_bytes, LVGL_PORT_BUFFER_MALLOC_CAPS);
    }
#endif

    ESP_UTILS_CHECK_NULL_RETURN(lvgl_buf[0], nullptr, "Allocate LVGL buffer 0 failed");
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_buf[1], nullptr, "Allocate LVGL buffer 1 failed");

    lv_display_t *disp = lv_display_create(lcd_width, lcd_height);
    ESP_UTILS_CHECK_NULL_RETURN(disp, nullptr, "Create LVGL display failed");

    lv_display_set_default(disp);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(disp, lcd);
    lv_display_set_flush_cb(disp, flush_callback);
    lv_display_set_buffers(disp, lvgl_buf[0], lvgl_buf[1], buffer_size_bytes, LVGL_PORT_RENDER_MODE);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

    return disp;
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    Touch *tp = static_cast<Touch *>(lv_indev_get_user_data(indev));
    TouchPoint point;
    data->state = LV_INDEV_STATE_RELEASED;

    if (tp->isInterruptEnabled() && xSemaphoreTake(touch_detected, 0) == pdFALSE) {
        return;
    }

    if (tp->readPoints(&point, 1, 0) > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

static bool on_touch_interrupt_callback(void *user_data)
{
    (void)user_data;
    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(touch_detected, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return false;
}

static lv_indev_t *indev_init(Touch *tp, lv_display_t *disp)
{
    ESP_UTILS_CHECK_FALSE_RETURN(tp != nullptr, nullptr, "Invalid touch device");
    ESP_UTILS_CHECK_FALSE_RETURN(tp->getPanelHandle() != nullptr, nullptr, "Touch device is not initialized");

    if (tp->isInterruptEnabled()) {
        touch_detected = xSemaphoreCreateBinary();
        ESP_UTILS_CHECK_NULL_RETURN(touch_detected, nullptr, "Create touch semaphore failed");
        tp->attachInterruptCallback(on_touch_interrupt_callback, tp);
    }

    lv_indev_t *indev = lv_indev_create();
    ESP_UTILS_CHECK_NULL_RETURN(indev, nullptr, "Create LVGL input device failed");
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);
    lv_indev_set_user_data(indev, tp);
    lv_indev_set_display(indev, disp);
    return indev;
}

static void tick_increment(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

static bool tick_init(void)
{
    const esp_timer_create_args_t timer_args = {
        .callback = tick_increment,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "LVGL tick",
        .skip_unhandled_events = false,
    };
    ESP_UTILS_CHECK_ERROR_RETURN(esp_timer_create(&timer_args, &lvgl_tick_timer), false, "Create LVGL tick timer failed");
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_start_periodic(lvgl_tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000), false,
        "Start LVGL tick timer failed"
    );
    return true;
}

static void lvgl_port_task(void *arg)
{
    (void)arg;
    uint32_t task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;

    while (true) {
        if (lvgl_port_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        task_delay_ms = constrain(task_delay_ms, LVGL_PORT_TASK_MIN_DELAY_MS, LVGL_PORT_TASK_MAX_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

bool lvgl_port_init(LCD *lcd, Touch *tp)
{
    ESP_UTILS_CHECK_FALSE_RETURN(lcd != nullptr, false, "Invalid LCD device");

    lv_init();
    ESP_UTILS_CHECK_FALSE_RETURN(tick_init(), false, "Initialize LVGL tick failed");

    lvgl_display = display_init(lcd);
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_display, false, "Initialize LVGL display failed");

#if !LVGL_PORT_AVOID_TEAR
    if (lcd->getBus()->getBasicAttributes().type != ESP_PANEL_BUS_TYPE_RGB) {
        lcd->attachDrawBitmapFinishCallback(on_draw_bitmap_finish_callback, lvgl_display);
    }
#endif

    if (tp != nullptr) {
        lvgl_indev = indev_init(tp, lvgl_display);
        ESP_UTILS_CHECK_NULL_RETURN(lvgl_indev, false, "Initialize LVGL input device failed");
    }

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "Create LVGL mutex failed");

    const BaseType_t core_id = LVGL_PORT_TASK_CORE < 0 ? tskNO_AFFINITY : LVGL_PORT_TASK_CORE;
    const BaseType_t ret = xTaskCreatePinnedToCore(
        lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, nullptr,
        LVGL_PORT_TASK_PRIORITY, &lvgl_task_handle, core_id
    );
    ESP_UTILS_CHECK_FALSE_RETURN(ret == pdPASS, false, "Create LVGL task failed");

#if LVGL_PORT_AVOID_TEAR
    lcd->attachRefreshFinishCallback(on_lcd_vsync_callback, lvgl_task_handle);
#endif
    return true;
}

bool lvgl_port_lock(int timeout_ms)
{
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "LVGL mutex is not initialized");
    const TickType_t timeout_ticks = timeout_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

bool lvgl_port_unlock(void)
{
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "LVGL mutex is not initialized");
    return xSemaphoreGiveRecursive(lvgl_mux) == pdTRUE;
}

bool lvgl_port_deinit(void)
{
    if (lvgl_tick_timer != nullptr) {
        esp_timer_stop(lvgl_tick_timer);
        esp_timer_delete(lvgl_tick_timer);
        lvgl_tick_timer = nullptr;
    }
    if (lvgl_task_handle != nullptr) {
        vTaskDelete(lvgl_task_handle);
        lvgl_task_handle = nullptr;
    }

    lv_deinit();

#if !LVGL_PORT_AVOID_TEAR
    for (void *&buffer : lvgl_buf) {
        free(buffer);
        buffer = nullptr;
    }
#endif
    if (touch_detected != nullptr) {
        vSemaphoreDelete(touch_detected);
        touch_detected = nullptr;
    }
    if (lvgl_mux != nullptr) {
        vSemaphoreDelete(lvgl_mux);
        lvgl_mux = nullptr;
    }
    lvgl_display = nullptr;
    lvgl_indev = nullptr;
    return true;
}
