/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include "esp_utils_conf_internal.h"
#include "esp_utils_mem_general.h"

namespace esp_utils {

template <typename T>
struct GeneralMemoryAllocator {
    using value_type = T;

    GeneralMemoryAllocator() = default;

    template <typename U>
    GeneralMemoryAllocator(const GeneralMemoryAllocator<U> &) {}

    bool operator!=(const GeneralMemoryAllocator &other) const
    {
        return false;
    }

    T *allocate(std::size_t n)
    {
        if (n == 0) {
            return nullptr;
        }
        void *ptr = esp_utils_mem_gen_malloc(n * sizeof(T));
#if !defined(ESP_PLATFORM) || CONFIG_COMPILER_CXX_EXCEPTIONS
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
#endif
        return static_cast<T *>(ptr);
    }

    void deallocate(T *p, std::size_t n)
    {
        esp_utils_mem_gen_free(p);
    }

    template <typename U, typename... Args>
    void construct(U *p, Args &&... args)
    {
        new (p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U *p)
    {
        p->~U();
    }

    template <typename U>
    struct rebind {
        using other = GeneralMemoryAllocator<U>;
    };
};

template <typename T, typename U>
bool operator==(const GeneralMemoryAllocator<T> &, const GeneralMemoryAllocator<U> &)
{
    return true;
}

template <typename T, typename U>
bool operator!=(const GeneralMemoryAllocator<T> &, const GeneralMemoryAllocator<U> &)
{
    return false;
}

} // namespace esp_utils
