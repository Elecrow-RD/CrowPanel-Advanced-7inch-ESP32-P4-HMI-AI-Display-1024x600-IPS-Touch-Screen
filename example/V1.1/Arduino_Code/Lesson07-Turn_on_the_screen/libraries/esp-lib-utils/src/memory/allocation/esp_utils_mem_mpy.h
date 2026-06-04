/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "py/mpconfig.h"
#include "py/misc.h"
#include "py/gc.h"

#define MALLOC(x)   gc_malloc(x)
#define FREE(x)     gc_free(x)
