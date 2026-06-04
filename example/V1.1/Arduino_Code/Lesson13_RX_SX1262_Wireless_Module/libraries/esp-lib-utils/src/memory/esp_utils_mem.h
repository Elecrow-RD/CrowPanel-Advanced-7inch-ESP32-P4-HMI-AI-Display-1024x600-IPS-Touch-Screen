/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include "esp_utils_mem_general.h"
#include "esp_utils_mem_cxx_global.h"
#ifdef __cplusplus
#include "esp_utils_mem_cxx_general.hpp"

extern "C" {
#endif

#if defined(ESP_PLATFORM)

/**
 * @brief Print memory information to the console
 *
 * This function prints the current memory usage information including:
 * - SRAM: Biggest free block, total free size, and total size
 * - PSRAM: Biggest free block, total free size, and total size
 *
 * @return true if the memory information was successfully printed, false otherwise
 */
bool esp_utils_mem_print_info(void);

#endif // ESP_PLATFORM

#ifdef __cplusplus
}
#endif
