/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdio.h>
#include <string.h>
#include "esp_utils_conf_internal.h"
#if ESP_UTILS_CONF_LOG_IMPL_TYPE == ESP_UTILS_LOG_IMPL_STDLIB
#   include "impl/esp_utils_log_impl_std.h"
#elif ESP_UTILS_CONF_LOG_IMPL_TYPE == ESP_UTILS_LOG_IMPL_ESP
#   include "impl/esp_utils_log_impl_esp.h"
#else
#   error "Invalid log implementation"
#endif

#ifndef ESP_UTILS_LOG_TAG
#   define ESP_UTILS_LOG_TAG "Utils"
#endif

#define ESP_UTILS_LOG_LEVEL(level, format, ...) do {                                                    \
        if      (level == ESP_UTILS_LOG_LEVEL_DEBUG)   { ESP_UTILS_LOGD_IMPL(ESP_UTILS_LOG_TAG, format, ##__VA_ARGS__); }  \
        else if (level == ESP_UTILS_LOG_LEVEL_INFO)    { ESP_UTILS_LOGI_IMPL(ESP_UTILS_LOG_TAG, format, ##__VA_ARGS__); }  \
        else if (level == ESP_UTILS_LOG_LEVEL_WARNING) { ESP_UTILS_LOGW_IMPL(ESP_UTILS_LOG_TAG, format, ##__VA_ARGS__); }  \
        else if (level == ESP_UTILS_LOG_LEVEL_ERROR)   { ESP_UTILS_LOGE_IMPL(ESP_UTILS_LOG_TAG, format, ##__VA_ARGS__); }  \
        else { }                                                                                        \
    } while(0)

#define ESP_UTILS_LOG_LEVEL_LOCAL(level, format, ...) do {                                          \
        if (level >= ESP_UTILS_CONF_LOG_LEVEL) ESP_UTILS_LOG_LEVEL(level, format, ##__VA_ARGS__); \
    } while(0)

/**
 * Macros to simplify logging calls
 */
#define ESP_UTILS_LOGD(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_DEBUG,   format, ##__VA_ARGS__)
#define ESP_UTILS_LOGI(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_INFO,    format, ##__VA_ARGS__)
#define ESP_UTILS_LOGW(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define ESP_UTILS_LOGE(format, ...) ESP_UTILS_LOG_LEVEL_LOCAL(ESP_UTILS_LOG_LEVEL_ERROR,   format, ##__VA_ARGS__)

/**
 * Micros to log trace of function calls
 */
#if ESP_UTILS_CONF_ENABLE_LOG_TRACE
#   define ESP_UTILS_LOG_TRACE_ENTER() ESP_UTILS_LOGD("Enter")
#   define ESP_UTILS_LOG_TRACE_EXIT()  ESP_UTILS_LOGD("Exit")
#else
#   define ESP_UTILS_LOG_TRACE_ENTER()
#   define ESP_UTILS_LOG_TRACE_EXIT()
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char *esp_utils_log_extract_file_name(const char *file_path);

#ifdef __cplusplus
}
#endif
