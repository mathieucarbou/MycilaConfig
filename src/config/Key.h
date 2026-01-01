// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#include <utility>
#include <variant>

#include "./Str.h"
#include "./Value.h"

namespace Mycila {
  namespace config {
    class Key {
      public:
        const char* name;
        Value defaultValue;
        Key(const char* n, Value&& dv) : name(n), defaultValue(std::move(dv)) {}
        Key(Key&& other) noexcept : name(other.name), defaultValue(std::move(other.defaultValue)) {}
        Key& operator=(Key&& other) noexcept {
          name = other.name;
          defaultValue = std::move(other.defaultValue);
          return *this;
        }
        Key(const Key&) = delete;
        Key& operator=(const Key&) = delete;

        bool isPasswordKey() const {
          size_t len = strlen(name);
          if (len < sizeof(MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) - 1)
            return false;
          return strcmp(name + len - sizeof(MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) - 1, MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) == 0;
        }

        bool isEnableKey() const {
          size_t len = strlen(name);
          if (len < sizeof(MYCILA_CONFIG_KEY_ENABLE_SUFFIX) - 1)
            return false;
          return strcmp(name + len - sizeof(MYCILA_CONFIG_KEY_ENABLE_SUFFIX) - 1, MYCILA_CONFIG_KEY_ENABLE_SUFFIX) == 0;
        }
    };
  } // namespace config
} // namespace Mycila
