/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <tuple>
#include <utility>

namespace esp_utils {

template<typename T>
class value_guard {
public:
    value_guard(T &value)
        : ref_(value)
        , origin_(value)
    {}

    ~value_guard()
    {
        if (!is_release_) {
            ref_ = origin_;
        }
    }

    void set(T value)
    {
        ref_ = value;
    }

    void release()
    {
        is_release_ = true;
    }

private:
    T &ref_;
    T origin_;
    bool is_release_ = false;
};

template<typename T, typename... Args>
class function_guard {
public:
    function_guard(T func, Args &&... args)
        : func_(func)
        , args_(std::forward<Args>(args)...)
    {}

    ~function_guard()
    {
        if (!is_release_) {
            std::apply([this](auto &&... args) {
                func_(std::forward<decltype(args)>(args)...);
            }, args_);
        }
    }

    void release()
    {
        is_release_ = true;
    }

private:
    T func_;
    std::tuple<Args...> args_;
    bool is_release_ = false;
};

} // namespace esp_utils
