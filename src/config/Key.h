// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
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
    };
  } // namespace config
} // namespace Mycila
