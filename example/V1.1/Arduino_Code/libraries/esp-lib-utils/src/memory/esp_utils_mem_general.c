/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "esp_utils_conf_internal.h"
#if ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_STD
#   include "allocation/esp_utils_mem_std.h"
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
#   define ESP_UTILS_MEM_ALLOC_ESP_ALIGN    ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN
#   define ESP_UTILS_MEM_ALLOC_ESP_CAPS     ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS
#   include "allocation/esp_utils_mem_esp.h"
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM
#   include ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE
#   define MALLOC(x)   ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_MALLOC(x)
#   define FREE(x)     ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_FREE(x)
#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_MICROPYTHON
#   include "allocation/esp_utils_mem_mpy.h"
#endif

static bool is_alloc_enabled = ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE;

void esp_utils_mem_gen_enable_alloc(bool enable)
{
    is_alloc_enabled = enable;
}

bool esp_utils_mem_gen_check_alloc_enabled(void)
{
    return is_alloc_enabled;
}

void *esp_utils_mem_gen_malloc(size_t size)
{
    void *p = NULL;

    if (!is_alloc_enabled) {
        p = malloc(size);
        goto end;
    }

    p = MALLOC(size);

end:
    return p;
}

void esp_utils_mem_gen_free(void *p)
{
    if (!is_alloc_enabled) {
        free(p);
        return;
    }
    FREE(p);
}

void *esp_utils_mem_gen_calloc(size_t n, size_t size)
{
    size_t total_size = (size_t)n * size;
    void *p = esp_utils_mem_gen_malloc(total_size);
    if (p != NULL) {
        memset(p, 0, total_size);
    }
    return p;
}
