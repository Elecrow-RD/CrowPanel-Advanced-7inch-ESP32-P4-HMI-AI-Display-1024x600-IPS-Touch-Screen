/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#if defined(ESP_PLATFORM)

#include "esp_heap_caps.h"

#if ESP_UTILS_MEM_ALLOC_ESP_CAPS == ESP_UTILS_MEM_ALLOC_ESP_CAPS_DEFAULT
#   define MEM_CAPS   MALLOC_CAP_DEFAULT
#elif ESP_UTILS_MEM_ALLOC_ESP_CAPS == ESP_UTILS_MEM_ALLOC_ESP_CAPS_SRAM
#   define MEM_CAPS   MALLOC_CAP_INTERNAL
#elif ESP_UTILS_MEM_ALLOC_ESP_CAPS == ESP_UTILS_MEM_ALLOC_ESP_CAPS_PSRAM
#   define MEM_CAPS   MALLOC_CAP_SPIRAM
#else
#   error "Invalid ESP memory caps"
#endif

#if !defined(ESP_UTILS_MEM_ALLOC_ESP_ALIGN)
#   error "Invalid ESP memory alignment"
#endif

#define MALLOC(x)   heap_caps_aligned_alloc(ESP_UTILS_MEM_ALLOC_ESP_ALIGN, x, MEM_CAPS)
#define FREE(x)     heap_caps_free(x)

#endif // ESP_PLATFORM
