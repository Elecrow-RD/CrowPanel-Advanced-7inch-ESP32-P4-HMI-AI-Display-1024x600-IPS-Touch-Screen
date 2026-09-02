/*
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#pragma once

#include <Arduino.h>
#include "esp_display_panel.hpp"
#include "lvgl.h"

#define LVGL_PORT_TICK_PERIOD_MS                (2)

#define LVGL_PORT_BUFFER_MALLOC_CAPS            (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#define LVGL_PORT_BUFFER_SIZE_HEIGHT            (20)
#define LVGL_PORT_BUFFER_NUM                    (2)

#define LVGL_PORT_TASK_MAX_DELAY_MS             (500)
#define LVGL_PORT_TASK_MIN_DELAY_MS             (2)
#define LVGL_PORT_TASK_STACK_SIZE               (6 * 1024)
#define LVGL_PORT_TASK_PRIORITY                 (2)
#ifdef ARDUINO_RUNNING_CORE
#define LVGL_PORT_TASK_CORE                     (ARDUINO_RUNNING_CORE)
#else
#define LVGL_PORT_TASK_CORE                     (0)
#endif

/*
 * 0: partial rendering without anti-tearing
 * 1: LCD double buffer + LVGL full rendering
 * 3: LCD double buffer + LVGL direct rendering
 */
#ifdef CONFIG_LVGL_PORT_AVOID_TEARING_MODE
#define LVGL_PORT_AVOID_TEARING_MODE            (CONFIG_LVGL_PORT_AVOID_TEARING_MODE)
#else
#define LVGL_PORT_AVOID_TEARING_MODE            (1)
#endif

#if LVGL_PORT_AVOID_TEARING_MODE != 0
#define LVGL_PORT_AVOID_TEAR                    (1)
#define LVGL_PORT_DISP_BUFFER_NUM               (2)
#if LVGL_PORT_AVOID_TEARING_MODE == 1
#define LVGL_PORT_RENDER_MODE                   LV_DISPLAY_RENDER_MODE_FULL
#elif LVGL_PORT_AVOID_TEARING_MODE == 3
#define LVGL_PORT_RENDER_MODE                   LV_DISPLAY_RENDER_MODE_DIRECT
#else
#error "LVGL 9 port supports anti-tearing mode 1 or 3"
#endif
#else
#define LVGL_PORT_AVOID_TEAR                    (0)
#define LVGL_PORT_RENDER_MODE                   LV_DISPLAY_RENDER_MODE_PARTIAL
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool lvgl_port_init(esp_panel::drivers::LCD *lcd, esp_panel::drivers::Touch *tp);
bool lvgl_port_deinit(void);
bool lvgl_port_lock(int timeout_ms);
bool lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif
