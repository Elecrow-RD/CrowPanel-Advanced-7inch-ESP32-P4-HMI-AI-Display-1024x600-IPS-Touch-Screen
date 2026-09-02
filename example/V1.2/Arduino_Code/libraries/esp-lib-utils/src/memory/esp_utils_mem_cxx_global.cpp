/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_utils_conf_internal.h"
#if ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
#include <atomic>
#include <new>
#if ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
#   define ESP_UTILS_MEM_ALLOC_ESP_ALIGN    ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_ALIGN
#   define ESP_UTILS_MEM_ALLOC_ESP_CAPS     ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_CAPS
#   include "allocation/esp_utils_mem_esp.h"
#elif ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM
#   include ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE
#   define MALLOC(x)   ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_NEW(x)
#   define FREE(x)     ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_DELETE(x)
#elif ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_MICROPYTHON
#   include "allocation/esp_utils_mem_mpy.h"
#endif // ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE

static std::atomic<bool> is_alloc_enabled = ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_DEFAULT_ENABLE;

void *operator new (std::size_t size)
{
    void *ptr = NULL;

    if (!is_alloc_enabled) {
        ptr = ::malloc(size);
        goto end;
    }

    ptr = MALLOC(size);
#if !defined(ESP_PLATFORM) || CONFIG_COMPILER_CXX_EXCEPTIONS
    if (!ptr) {
        throw std::bad_alloc();
    }
#endif

end:
    return ptr;
}

void *operator new[](std::size_t size)
{
    void *ptr = NULL;

    if (!is_alloc_enabled) {
        ptr = ::malloc(size);
        goto end;
    }

    ptr = MALLOC(size);
#if !defined(ESP_PLATFORM) || CONFIG_COMPILER_CXX_EXCEPTIONS
    if (!ptr) {
        throw std::bad_alloc();
    }
#endif

end:
    return ptr;
}

void operator delete (void *ptr) noexcept
{
    if (!is_alloc_enabled) {
        ::free(ptr);
        return;
    }

    FREE(ptr);
}

void operator delete[](void *ptr) noexcept
{
    if (!is_alloc_enabled) {
        ::free(ptr);
        return;
    }

    FREE(ptr);
}

void operator delete (void *ptr, std::size_t size) noexcept
{
    if (!is_alloc_enabled) {
        ::free(ptr);
        return;
    }

    FREE(ptr);
}

void operator delete[](void *ptr, std::size_t size) noexcept
{
    if (!is_alloc_enabled) {
        ::free(ptr);
        return;
    }

    FREE(ptr);
}

void esp_utils_mem_cxx_glob_enable_alloc(bool enable)
{
    is_alloc_enabled = enable;
}

bool esp_utils_mem_cxx_glob_check_alloc_enabled(void)
{
    return is_alloc_enabled;
}

#endif // ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
