// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <variant>

#include "./Defines.h"
#include "./Str.h"

namespace Mycila {
  namespace config {
    class Value : public std::variant<
                    bool,
                    int8_t, uint8_t,
                    int16_t, uint16_t,
                    int32_t, uint32_t,
#if MYCILA_CONFIG_USE_LONG_LONG
                    int64_t, uint64_t,
#endif
                    int, unsigned int,
                    float,
#if MYCILA_CONFIG_USE_DOUBLE
                    double,
#endif
                    Str> {
      private:
        using Base = std::variant<
          bool,
          int8_t, uint8_t,
          int16_t, uint16_t,
          int32_t, uint32_t,
#if MYCILA_CONFIG_USE_LONG_LONG
          int64_t, uint64_t,
#endif
          int, unsigned int,
          float,
#if MYCILA_CONFIG_USE_DOUBLE
          double,
#endif
          Str>;

      public:
        // Inherit all constructors from std::variant
        using Base::Base;
        using Base::operator=;

        // get the value held as std::string (only for Str type)
        template <typename T, std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
        std::string as() const {
          if (std::holds_alternative<Str>(*this)) {
            return std::string{std::get<Str>(*this).c_str()};
          }
          throw std::runtime_error("Invalid type conversion");
        }

        // get the value held as const char* (only for Str type)
        template <typename T, std::enable_if_t<std::is_same_v<T, const char*>, int> = 0>
        const char* as() const {
          if (std::holds_alternative<Str>(*this)) {
            return std::get<Str>(*this).c_str();
          }
          throw std::runtime_error("Invalid type conversion");
        }

        // get the value held as type T (for variant alternatives or Value itself)
        template <typename T = Value, std::enable_if_t<!std::is_same_v<T, std::string> && !std::is_same_v<T, const char*>, int> = 0>
        const T& as() const {
          if constexpr (std::is_same_v<T, Value>) {
            return *this;
          } else if (std::holds_alternative<T>(*this)) {
            return std::get<T>(*this);
          }
          throw std::runtime_error("Invalid type conversion");
        }

        // convert to a string any value held
        std::string toString() const {
          return std::visit(
            [](auto&& arg) -> std::string {
              using T = std::decay_t<decltype(arg)>;
              if constexpr (std::is_same_v<T, bool>) {
                return arg ? MYCILA_CONFIG_VALUE_TRUE : MYCILA_CONFIG_VALUE_FALSE;
              } else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(arg);
              } else if constexpr (std::is_same_v<T, Str>) {
                return std::string(arg.c_str());
              } else {
                return std::string{};
              }
            },
            *this);
        }

        static std::optional<Value> fromString(const char* str, const Value& defaultValue) {
          return std::visit(
            [&](auto&& variant) -> std::optional<Value> {
              using T = std::decay_t<decltype(variant)>;

              if constexpr (std::is_same_v<T, bool>) {
                if (strcmp(str, MYCILA_CONFIG_VALUE_TRUE) == 0) {
                  return true;
                }
#if MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING
                if (strcmp(str, "true") == 0 || strcmp(str, "1") == 0 ||
                    strcmp(str, "on") == 0 || strcmp(str, "yes") == 0 || strcmp(str, "y") == 0) {
                  return true;
                }
#endif
                return false;
              }

              if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
                            std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
                char* endPtr;
                auto val = std::strtol(str, &endPtr, 10);
                if (*endPtr == '\0') {
                  return static_cast<T>(val);
                }
              }

              if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                            std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
                char* endPtr;
                auto val = strtoul(str, &endPtr, 10);
                if (*endPtr == '\0') {
                  return static_cast<T>(val);
                }
              }

#if MYCILA_CONFIG_USE_LONG_LONG
              if constexpr (std::is_same_v<T, int64_t>) {
                char* endPtr;
                auto val = std::strtoll(str, &endPtr, 10);
                if (*endPtr == '\0') {
                  return static_cast<T>(val);
                }
              }

              if constexpr (std::is_same_v<T, uint64_t>) {
                char* endPtr;
                auto val = std::strtoull(str, &endPtr, 10);
                if (*endPtr == '\0') {
                  return static_cast<T>(val);
                }
              }
#endif

              if constexpr (std::is_same_v<T, float>) {
                char* endPtr;
                auto val = std::strtof(str, &endPtr);
                if (*endPtr == '\0') {
                  return static_cast<T>(val);
                }
              }

#if MYCILA_CONFIG_USE_DOUBLE
              if constexpr (std::is_same_v<T, double>) {
                char* endPtr;
                auto val = std::strtod(str, &endPtr);
                if (*endPtr == '\0') {
                  return static_cast<T>(val);
                }
              }
#endif

              if constexpr (std::is_same_v<T, Str>) {
                return Str(str);
              }

              return std::nullopt;
            },
            defaultValue);
        }
    };
  } // namespace config
} // namespace Mycila
