// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

// #include <esp_memory_utils.h>
#include <soc/soc.h>
#include <string.h>

#include <utility>

namespace Mycila {
  namespace config {
    class Str {
      public:
        Str() : Str("") {}
        Str(size_t length) { // NOLINT
          _buffer = new char[length + 1];
          _buffer[0] = '\0';
        }
        Str(const char* str) { // NOLINT
          if (_isFlashString(str)) {
            _buffer = const_cast<char*>(str);
          } else {
            _buffer = new char[strlen(str) + 1];
            strcpy(_buffer, str); // NOLINT
          }
        }
        ~Str() {
          if (!_isFlashString(_buffer)) {
            delete[] _buffer;
          }
        }
        Str(Str&& other) noexcept : _buffer(other._buffer) { other._buffer = nullptr; }
        Str& operator=(Str&& other) noexcept {
          if (this != &other) {
            if (_buffer && !_isFlashString(_buffer))
              delete[] _buffer;
            _buffer = other._buffer;
            other._buffer = nullptr;
          }
          return *this;
        }
        Str(const Str& other) {
          if (other._buffer) {
            if (_isFlashString(other._buffer)) {
              _buffer = other._buffer;
            } else {
              _buffer = new char[strlen(other._buffer) + 1];
              strcpy(_buffer, other._buffer); // NOLINT
            }
          }
        }
        Str& operator=(const Str& other) {
          if (this != &other) {
            if (!_isFlashString(_buffer)) {
              delete[] _buffer;
            }
            _buffer = nullptr;
            if (other._buffer) {
              if (_isFlashString(other._buffer)) {
                _buffer = other._buffer;
              } else {
                _buffer = new char[strlen(other._buffer) + 1];
                strcpy(_buffer, other._buffer); // NOLINT
              }
            }
          }
          return *this;
        }
        bool inFlash() const { return _isFlashString(_buffer); }
        const char* c_str() const { return _buffer; }
        char* buffer() const { return _buffer; }
        size_t heapUsage() const { return _isFlashString(_buffer) ? 0 : strlen(_buffer) + 1; }
        friend bool operator==(const Str& lhs, const Str& rhs) { return lhs._buffer == rhs._buffer || strcmp(lhs._buffer, rhs._buffer) == 0; }
        friend bool operator==(const Str& lhs, const char* rhs) { return lhs._buffer == rhs || strcmp(lhs._buffer, rhs) == 0; }
        friend bool operator==(const char* lhs, const Str& rhs) { return lhs == rhs._buffer || strcmp(lhs, rhs._buffer) == 0; }
        bool empty() const { return !_buffer || _buffer[0] == '\0'; }
        size_t length() const { return _buffer ? strlen(_buffer) : 0; }

      private:
        char* _buffer = nullptr;

        __attribute__((always_inline)) inline static bool _isFlashString(const char* str) { return _my_esp_ptr_in_drom(str) || _my_esp_ptr_in_rom(str); }

        // Copy of ESP-IDF esp_ptr_in_drom in #include <esp_memory_utils.h>
        __attribute__((always_inline)) inline static bool _my_esp_ptr_in_rom(const void* p) {
          intptr_t ip = (intptr_t)p;
          return
#if CONFIG_IDF_TARGET_ARCH_RISCV && SOC_DROM_MASK_LOW != SOC_IROM_MASK_LOW
            (ip >= SOC_DROM_MASK_LOW && ip < SOC_DROM_MASK_HIGH) ||
#endif
            (ip >= SOC_IROM_MASK_LOW && ip < SOC_IROM_MASK_HIGH);
        }

        // Copy of ESP-IDF esp_ptr_in_rom in #include <esp_memory_utils.h>
        __attribute__((always_inline)) inline static bool _my_esp_ptr_in_drom(const void* p) {
          int32_t drom_start_addr = SOC_DROM_LOW;
#if CONFIG_ESP32S3_DATA_CACHE_16KB
          drom_start_addr += 0x4000;
#endif

          return ((intptr_t)p >= drom_start_addr && (intptr_t)p < SOC_DROM_HIGH);
        }
    };
  } // namespace config
} // namespace Mycila
