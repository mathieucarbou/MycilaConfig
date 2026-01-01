// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#include <nvs.h>
#include <nvs_flash.h>

#include <optional>

#include "./config/Storage.h"

namespace Mycila {
  namespace config {
    class NVS : public Storage {
      public:
        NVS() = default;
        virtual ~NVS() { end(); }

        bool begin(const char* name) override {
          if (_handle)
            return true;
          if (nvs_open(name, NVS_READWRITE, &_handle) != ESP_OK) {
            _handle = 0;
            return false;
          }
          return true;
        }

        void end() override {
          if (_handle) {
            nvs_close(_handle);
            _handle = 0;
          }
        }

        // Note: this call is very inefficient as it tries to read all possible types
        bool hasKey(const char* key) const override {
          if (!_handle)
            return false;
          int8_t mt1;
          uint8_t mt2;
          int16_t mt3;
          uint16_t mt4;
          int32_t mt5;
          uint32_t mt6;
          int64_t mt7;
          uint64_t mt8;
          size_t len = 0;
          // clang-format off
          return nvs_get_i8(_handle, key, &mt1) == ESP_OK
            || nvs_get_u8(_handle, key, &mt2) == ESP_OK
            || nvs_get_i16(_handle, key, &mt3) == ESP_OK
            || nvs_get_u16(_handle, key, &mt4) == ESP_OK
            || nvs_get_i32(_handle, key, &mt5) == ESP_OK
            || nvs_get_u32(_handle, key, &mt6) == ESP_OK
            || nvs_get_i64(_handle, key, &mt7) == ESP_OK
            || nvs_get_u64(_handle, key, &mt8) == ESP_OK
            || nvs_get_str(_handle, key, NULL, &len) == ESP_OK
            || nvs_get_blob(_handle, key, NULL, &len) == ESP_OK;
          // clang-format on
        };

        bool remove(const char* key) override {
          if (!_handle)
            return false;
          esp_err_t err = nvs_erase_key(_handle, key);
          if (err == ESP_ERR_NVS_NOT_FOUND)
            return true; // key already removed
          return err == ESP_OK && nvs_commit(_handle) == ESP_OK;
        }

        bool removeAll() override { return _handle && nvs_erase_all(_handle) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeBool(const char* key, bool value) override { return _handle && nvs_set_u8(_handle, key, value ? 1 : 0) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeFloat(const char* key, float value) override { return _handle && nvs_set_blob(_handle, key, &value, sizeof(float)) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeDouble(const char* key, double value) override { return _handle && nvs_set_blob(_handle, key, &value, sizeof(double)) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeI8(const char* key, int8_t value) override { return _handle && nvs_set_i8(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeU8(const char* key, uint8_t value) override { return _handle && nvs_set_u8(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeI16(const char* key, int16_t value) override { return _handle && nvs_set_i16(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeU16(const char* key, uint16_t value) override { return _handle && nvs_set_u16(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeI32(const char* key, int32_t value) override { return _handle && nvs_set_i32(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeU32(const char* key, uint32_t value) override { return _handle && nvs_set_u32(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeI64(const char* key, int64_t value) override { return _handle && nvs_set_i64(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeU64(const char* key, uint64_t value) override { return _handle && nvs_set_u64(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }
        bool storeString(const char* key, const char* value) override { return _handle && value && nvs_set_str(_handle, key, value) == ESP_OK && nvs_commit(_handle) == ESP_OK; }

        std::optional<bool> loadBool(const char* key) const override {
          uint8_t value;
          if (_handle && nvs_get_u8(_handle, key, &value) == ESP_OK)
            return value != 0;
          return std::nullopt;
        }
        std::optional<float> loadFloat(const char* key) const override {
          float value;
          size_t len = sizeof(float);
          if (_handle && nvs_get_blob(_handle, key, &value, &len) == ESP_OK && len == sizeof(float))
            return value;
          return std::nullopt;
        }
        std::optional<double> loadDouble(const char* key) const override {
          double value;
          size_t len = sizeof(double);
          if (_handle && nvs_get_blob(_handle, key, &value, &len) == ESP_OK && len == sizeof(double))
            return value;
          return std::nullopt;
        }
        std::optional<int8_t> loadI8(const char* key) const override {
          int8_t value;
          if (_handle && nvs_get_i8(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<uint8_t> loadU8(const char* key) const override {
          uint8_t value;
          if (_handle && nvs_get_u8(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<int16_t> loadI16(const char* key) const override {
          int16_t value;
          if (_handle && nvs_get_i16(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<uint16_t> loadU16(const char* key) const override {
          uint16_t value;
          if (_handle && nvs_get_u16(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<int32_t> loadI32(const char* key) const override {
          int32_t value;
          if (_handle && nvs_get_i32(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<uint32_t> loadU32(const char* key) const override {
          uint32_t value;
          if (_handle && nvs_get_u32(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<int64_t> loadI64(const char* key) const override {
          int64_t value;
          if (_handle && nvs_get_i64(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<uint64_t> loadU64(const char* key) const override {
          uint64_t value;
          if (_handle && nvs_get_u64(_handle, key, &value) == ESP_OK)
            return value;
          return std::nullopt;
        }
        std::optional<Str> loadString(const char* key) const override {
          if (!_handle)
            return std::nullopt;
          size_t len = 0;
          esp_err_t err = nvs_get_str(_handle, key, NULL, &len);
          if (err != ESP_OK || !len)
            return std::nullopt;
          Str str(len - 1);
          err = nvs_get_str(_handle, key, str.buffer(), &len);
          if (err != ESP_OK) {
            return std::nullopt;
          }
          return str;
        }

      private:
        uint32_t _handle;
    };
  } // namespace config
} // namespace Mycila
