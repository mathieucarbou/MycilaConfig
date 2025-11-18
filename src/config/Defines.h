// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#define MYCILA_CONFIG_VERSION          "10.0.0"
#define MYCILA_CONFIG_VERSION_MAJOR    10
#define MYCILA_CONFIG_VERSION_MINOR    0
#define MYCILA_CONFIG_VERSION_REVISION 0

#define MYCILA_CONFIG_LOG_TAG "CONFIG"

#ifndef MYCILA_CONFIG_VALUE_TRUE
  #define MYCILA_CONFIG_VALUE_TRUE "true"
#endif

#ifndef MYCILA_CONFIG_VALUE_FALSE
  #define MYCILA_CONFIG_VALUE_FALSE "false"
#endif

#ifndef MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING
  #define MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING 1
#endif

#ifndef MYCILA_CONFIG_SHOW_PASSWORD
  #ifndef MYCILA_CONFIG_PASSWORD_MASK
    #define MYCILA_CONFIG_PASSWORD_MASK "********"
  #endif
#endif

// suffix to use for a setting key enabling a feature
#ifndef MYCILA_CONFIG_KEY_ENABLE_SUFFIX
  #define MYCILA_CONFIG_KEY_ENABLE_SUFFIX "_enable"
#endif

// suffix to use for a setting key representing a password
#ifndef MYCILA_CONFIG_KEY_PASSWORD_SUFFIX
  #define MYCILA_CONFIG_KEY_PASSWORD_SUFFIX "_pwd"
#endif

// support for int64_t and uint64_t types
#ifndef MYCILA_CONFIG_USE_LONG_LONG
  #define MYCILA_CONFIG_USE_LONG_LONG 0
#endif

// support for double type
#ifndef MYCILA_CONFIG_USE_DOUBLE
  #define MYCILA_CONFIG_USE_DOUBLE 0
#endif
