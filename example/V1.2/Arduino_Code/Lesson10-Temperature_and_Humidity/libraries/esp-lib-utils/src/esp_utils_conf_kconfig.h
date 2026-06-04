/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Check Configurations /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ESP_UTILS_CONF_CHECK_HANDLE_METHOD
#   ifdef CONFIG_ESP_UTILS_CONF_CHECK_HANDLE_METHOD
#       define ESP_UTILS_CONF_CHECK_HANDLE_METHOD   CONFIG_ESP_UTILS_CONF_CHECK_HANDLE_METHOD
#   else
#       error "Missing configuration: ESP_UTILS_CONF_CHECK_HANDLE_METHOD"
#   endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// LOG Configurations //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ESP_UTILS_CONF_LOG_IMPL_TYPE
#   ifdef CONFIG_ESP_UTILS_CONF_LOG_IMPL_TYPE
#       define ESP_UTILS_CONF_LOG_IMPL_TYPE      CONFIG_ESP_UTILS_CONF_LOG_IMPL_TYPE
#   else
#       define ESP_UTILS_CONF_LOG_IMPL_TYPE      ESP_UTILS_LOG_IMPL_STDLIB
#   endif
#endif

#ifndef ESP_UTILS_CONF_LOG_LEVEL
#   ifdef CONFIG_ESP_UTILS_CONF_LOG_LEVEL
#       define ESP_UTILS_CONF_LOG_LEVEL      CONFIG_ESP_UTILS_CONF_LOG_LEVEL
#   else
#       error "Missing configuration: ESP_UTILS_CONF_LOG_LEVEL"
#   endif
#endif

#if ESP_UTILS_CONF_LOG_LEVEL == ESP_UTILS_LOG_LEVEL_DEBUG
#   ifndef ESP_UTILS_CONF_ENABLE_LOG_TRACE
#       ifdef CONFIG_ESP_UTILS_CONF_ENABLE_LOG_TRACE
#           define ESP_UTILS_CONF_ENABLE_LOG_TRACE      CONFIG_ESP_UTILS_CONF_ENABLE_LOG_TRACE
#       else
#           define ESP_UTILS_CONF_ENABLE_LOG_TRACE      0
#       endif
#   endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Memory Configurations /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * C/C++ general allocator
 */
#ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE
#   ifdef CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE
#       define ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE        CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE
#   else
#       define ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE        (0)
#   endif
#endif

#ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE
#   ifdef CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE
#       define ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE        CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE
#   else
#       error "Missing configuration: ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE"
#   endif
#endif

#if ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
#   ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN
#       ifdef CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN
#           define ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN  CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN
#       else
#           error "Missing configuration: ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN"
#       endif
#   endif

#   ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS
#       ifdef CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS
#           define ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS        CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS
#       else
#           error "Missing configuration: ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS"
#       endif
#   endif

#elif ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM

#   ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE
#       ifdef CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE
#           define ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE  CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE
#       else
#           error "Missing configuration: ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE"
#       endif
#   endif

#   ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_MALLOC
#       error "`ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_MALLOC` must be defined when using C/C++ custom general allocator"
#   endif

#   ifndef ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_FREE
#       error "`ESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_FREE` must be defined when using C/C++ custom general allocator"
#   endif
#endif

/**
 * C++ global allocator
 */
#ifndef ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
#   ifdef CONFIG_ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
#       define ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC        CONFIG_ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
#   else
#       define ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC        (0)
#   endif
#endif

#if ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC
#   ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_DEFAULT_ENABLE
#       ifdef CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_DEFAULT_ENABLE
#           define ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_DEFAULT_ENABLE        CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_DEFAULT_ENABLE
#       else
#           define ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_DEFAULT_ENABLE        (0)
#       endif
#   endif

#   ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE
#       ifdef CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE
#           define ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE        CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE
#       else
#           error "Missing configuration: ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE"
#       endif
#   endif

#   if ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_ESP
#       ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_ALIGN
#           ifdef CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_ALIGN
#               define ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_ALIGN  CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_ALIGN
#           else
#               error "Missing configuration: ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_ALIGN"
#           endif
#       endif

#       ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_CAPS
#           ifdef CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_CAPS
#               define ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_CAPS        CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_CAPS
#           else
#               error "Missing configuration: ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_ESP_CAPS"
#           endif
#       endif

#   elif ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE == ESP_UTILS_MEM_ALLOC_TYPE_CUSTOM

#       ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE
#           ifdef CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE
#               define ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE  CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE
#           else
#               error "Missing configuration: ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE"
#           endif
#       endif

#       ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_NEW
#           error "`ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_NEW` must be defined when using C++ custom global allocator"
#       endif

#       ifndef ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_DELETE
#           error "`ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_DELETE` must be defined when using C++ custom global allocator"
#       endif
#   endif
#endif
