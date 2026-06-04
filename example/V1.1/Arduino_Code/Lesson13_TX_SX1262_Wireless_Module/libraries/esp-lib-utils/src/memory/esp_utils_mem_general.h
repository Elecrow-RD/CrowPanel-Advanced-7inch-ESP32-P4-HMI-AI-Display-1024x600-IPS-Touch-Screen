/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable or disable the general memory allocator
 *
 * @param[in] enable true to enable, false to disable
 */
void esp_utils_mem_gen_enable_alloc(bool enable);

/**
 * @brief Check if the general memory allocator is enabled
 *
 * @param[in] size Size parameter (used for compatibility with allocation functions)
 * @return void* Pointer to indicate allocation status
 */
void *esp_utils_mem_gen_check_alloc_enabled(size_t size);

/**
 * @brief Allocate memory using the general memory allocator
 *
 * @param[in] size Size of memory to allocate in bytes
 * @return void* Pointer to allocated memory or NULL if allocation fails
 */
void *esp_utils_mem_gen_malloc(size_t size);

/**
 * @brief Free memory allocated by the general memory allocator
 *
 * @param[in] p Pointer to memory to free
 */
void esp_utils_mem_gen_free(void *p);

/**
 * @brief Allocate and zero-initialize memory using the general memory allocator
 *
 * @param[in] n Number of elements to allocate
 * @param[in] size Size of each element in bytes
 * @return void* Pointer to allocated memory or NULL if allocation fails
 */
void *esp_utils_mem_gen_calloc(size_t n, size_t size);

#ifdef __cplusplus
}
#endif
