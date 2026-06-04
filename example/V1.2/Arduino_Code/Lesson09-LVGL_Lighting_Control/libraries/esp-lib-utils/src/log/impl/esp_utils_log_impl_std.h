/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdio.h>

#define ESP_UTILS_LOGD_IMPL_FUNC(TAG, format, ...)         printf("[D][%s]" format "\n", TAG, ##__VA_ARGS__)
#define ESP_UTILS_LOGI_IMPL_FUNC(TAG, format, ...)         printf("[I][%s]" format "\n", TAG, ##__VA_ARGS__)
#define ESP_UTILS_LOGW_IMPL_FUNC(TAG, format, ...)         printf("[W][%s]" format "\n", TAG, ##__VA_ARGS__)
#define ESP_UTILS_LOGE_IMPL_FUNC(TAG, format, ...)         printf("[E][%s]" format "\n", TAG, ##__VA_ARGS__)

#define ESP_UTILS_LOGD_IMPL(TAG, format, ...) ESP_UTILS_LOGD_IMPL_FUNC(TAG, "[%s:%04d](%s): " format, \
                                        esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
#define ESP_UTILS_LOGI_IMPL(TAG, format, ...) ESP_UTILS_LOGI_IMPL_FUNC(TAG, "[%s:%04d](%s): " format, \
                                        esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
#define ESP_UTILS_LOGW_IMPL(TAG, format, ...) ESP_UTILS_LOGW_IMPL_FUNC(TAG, "[%s:%04d](%s): " format, \
                                        esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
#define ESP_UTILS_LOGE_IMPL(TAG, format, ...) ESP_UTILS_LOGE_IMPL_FUNC(TAG, "[%s:%04d](%s): " format, \
                                        esp_utils_log_extract_file_name(__FILE__), __LINE__, __func__,  ##__VA_ARGS__)
