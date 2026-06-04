/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cstring>
#include <string>
#include <string_view>
#include "check/esp_utils_check.h"

/**
 * @brief Extract filename from file path
 *
 * @param[in] file_path File path
 *
 * @return File name
 */
extern "C" const char *esp_utils_log_extract_file_name(const char *file_path)
{
    ESP_UTILS_CHECK_NULL_RETURN(file_path, nullptr, "File path is null");

    const char *filename = strrchr(file_path, '/');
    if (!filename) {
        filename = strrchr(file_path, '\\');  // Windows path compatibility
    }

    return filename ? filename + 1 : file_path;
}

namespace esp_utils {
namespace detail {

std::string parseFunctionName(const char *func_name)
{
    std::string_view sig(func_name);

    // Part before the first '(' parenthesis
    size_t paren_pos = sig.find('(');
    if (paren_pos == std::string_view::npos) {
        return {};
    }

    std::string_view before_paren = sig.substr(0, paren_pos);

    // Find the last "::"
    size_t last_colon = before_paren.rfind("::");
    if (last_colon == std::string_view::npos) {
        // Case without ::, find the last space
        size_t last_space = before_paren.rfind(' ');
        if (last_space == std::string_view::npos) {
            return std::string(before_paren); // No space found, return entire part
        }
        std::string_view func_name_result = before_paren.substr(last_space + 1);

        return std::string(func_name_result);
    }

    std::string_view func_name_result = before_paren.substr(last_colon + 2);

    return std::string(func_name_result);
}

} // namespace detail
} // namespace esp_utils
