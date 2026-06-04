/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include "esp_utils_conf_internal.h"

#if ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable or disable the global C++ memory allocator
 *
 * @param[in] enable true to enable, false to disable
 */
void esp_utils_mem_cxx_glob_enable_alloc(bool enable);

/**
 * @brief Check if the global C++ memory allocator is enabled
 *
 * @return true if enabled, false otherwise
 */
bool esp_utils_mem_cxx_glob_check_alloc_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
