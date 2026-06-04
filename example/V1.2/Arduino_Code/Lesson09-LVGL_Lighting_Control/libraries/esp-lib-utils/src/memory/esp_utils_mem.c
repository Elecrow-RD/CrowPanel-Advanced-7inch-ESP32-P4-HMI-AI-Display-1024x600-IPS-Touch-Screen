/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(ESP_PLATFORM)

#include "esp_heap_caps.h"
#include "check/esp_utils_check.h"
#include "log/esp_utils_log.h"
#include "esp_utils_mem.h"

#define PRINT_INFO_BUFFER_SIZE  256

bool esp_utils_mem_print_info(void)
{
    char *buffer = esp_utils_mem_gen_calloc(1, PRINT_INFO_BUFFER_SIZE);
    ESP_UTILS_CHECK_NULL_RETURN(buffer, false, "Allocate buffer failed");

    snprintf(
        buffer, PRINT_INFO_BUFFER_SIZE,
        "ESP Memory Info:\n"
        "          Biggest /     Free /    Total\n"
        " SRAM : [%8d / %8d / %8d]\n"
        "PSRAM : [%8d / %8d / %8d]",
        (int)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (int)heap_caps_get_total_size(MALLOC_CAP_INTERNAL),
        (int)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        (int)heap_caps_get_total_size(MALLOC_CAP_SPIRAM)
    );
    printf("%s\n", buffer);

    esp_utils_mem_gen_free(buffer);

    return true;
}

#endif // ESP_PLATFORM
