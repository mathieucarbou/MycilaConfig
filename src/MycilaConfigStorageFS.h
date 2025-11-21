// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <FS.h>

#include <optional>
#include <string>

#include "./config/Storage.h"

namespace Mycila {
  namespace config {
    class FileSystem : public Storage {
      public:
        FileSystem() = default;
        virtual ~FileSystem() { end(); }

        void setFS(FS* fs) { _fs = fs; }

        bool begin(const char* name) override {
          if (!_fs)
            return false;

          if (name[0] == '\0')
            return false;

          std::string dir = name[0] == '/' ? std::string(name) : std::string("/") + name;

          if (!_fs->exists(dir.c_str())) {
            if (!_fs->mkdir(dir.c_str())) {
              return false;
            }
          }

          _root = dir + "/";

          return true;
        }

        void end() override {}

        bool hasKey(const char* key) const override {
          // ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "hasKey(%s)", (_root + key).c_str());
          return _fs && _fs->exists((_root + key).c_str());
        };

        bool remove(const char* key) override {
          // ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "remove(%s)", (_root + key).c_str());
          return _fs && _fs->remove((_root + key).c_str());
        }

        bool removeAll() override {
          if (!_fs)
            return false;

          if (!_fs->exists(_root.c_str())) {
            return true; // nothing to remove
          }

          File dir = _fs->open(_root.c_str());
          if (!dir || !dir.isDirectory()) {
            dir.close();
            return false;
          }

          File file = dir.openNextFile();
          while (file) {
            String path = file.path();
            file.close();

            if (!_fs->remove(path.c_str())) {
              dir.close();
              return false;
            }

            file = dir.openNextFile();
          }

          dir.close();
          return true;
        }

        bool storeBool(const char* key, bool value) override { return _store(key, "bool", value ? "true" : "false"); }
        bool storeFloat(const char* key, float value) override { return _store(key, "float", std::to_string(value).c_str()); }
        bool storeDouble(const char* key, double value) override { return _store(key, "double", std::to_string(value).c_str()); }
        bool storeI8(const char* key, int8_t value) override { return _store(key, "int8", std::to_string(value).c_str()); }
        bool storeU8(const char* key, uint8_t value) override { return _store(key, "uint8", std::to_string(value).c_str()); }
        bool storeI16(const char* key, int16_t value) override { return _store(key, "int16", std::to_string(value).c_str()); }
        bool storeU16(const char* key, uint16_t value) override { return _store(key, "uint16", std::to_string(value).c_str()); }
        bool storeI32(const char* key, int32_t value) override { return _store(key, "int32", std::to_string(value).c_str()); }
        bool storeU32(const char* key, uint32_t value) override { return _store(key, "uint32", std::to_string(value).c_str()); }
        bool storeI64(const char* key, int64_t value) override { return _store(key, "int64", std::to_string(value).c_str()); }
        bool storeU64(const char* key, uint64_t value) override { return _store(key, "uint64", std::to_string(value).c_str()); }
        bool storeString(const char* key, const char* value) override { return _store(key, "string", value); }

        std::optional<bool> loadBool(const char* key) const override {
          std::optional<String> str = _load(key, "bool");
          if (!str.has_value())
            return std::nullopt;
          return str == "true";
        }
        std::optional<float> loadFloat(const char* key) const override {
          std::optional<String> str = _load(key, "float");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          auto val = strtof(str->c_str(), &endPtr);
          if (*endPtr == '\0') {
            return val;
          }
          return std::nullopt;
        }
        std::optional<double> loadDouble(const char* key) const override {
          std::optional<String> str = _load(key, "double");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          auto val = strtod(str->c_str(), &endPtr);
          if (*endPtr == '\0') {
            return val;
          }
          return std::nullopt;
        }
        std::optional<int8_t> loadI8(const char* key) const override {
          std::optional<String> str = _load(key, "int8");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          long val = strtol(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0' && val >= INT8_MIN && val <= INT8_MAX) {
            return static_cast<int8_t>(val);
          }
          return std::nullopt;
        }
        std::optional<uint8_t> loadU8(const char* key) const override {
          std::optional<String> str = _load(key, "uint8");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          unsigned long val = strtoul(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0' && val <= UINT8_MAX) {
            return static_cast<uint8_t>(val);
          }
          return std::nullopt;
        }
        std::optional<int16_t> loadI16(const char* key) const override {
          std::optional<String> str = _load(key, "int16");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          long val = strtol(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0' && val >= INT16_MIN && val <= INT16_MAX) {
            return static_cast<int16_t>(val);
          }
          return std::nullopt;
        }
        std::optional<uint16_t> loadU16(const char* key) const override {
          std::optional<String> str = _load(key, "uint16");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          unsigned long val = strtoul(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0' && val <= UINT16_MAX) {
            return static_cast<uint16_t>(val);
          }
          return std::nullopt;
        }
        std::optional<int32_t> loadI32(const char* key) const override {
          std::optional<String> str = _load(key, "int32");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          long val = strtol(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0' && val >= INT32_MIN && val <= INT32_MAX) {
            return static_cast<int32_t>(val);
          }
          return std::nullopt;
        }
        std::optional<uint32_t> loadU32(const char* key) const override {
          std::optional<String> str = _load(key, "uint32");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          unsigned long val = strtoul(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0' && val <= UINT32_MAX) {
            return static_cast<uint32_t>(val);
          }
          return std::nullopt;
        }
        std::optional<int64_t> loadI64(const char* key) const override {
          std::optional<String> str = _load(key, "int64");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          long long val = strtoll(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0') {
            return static_cast<int64_t>(val);
          }
          return std::nullopt;
        }
        std::optional<uint64_t> loadU64(const char* key) const override {
          std::optional<String> str = _load(key, "uint64");
          if (!str.has_value())
            return std::nullopt;
          char* endPtr;
          unsigned long long val = strtoull(str->c_str(), &endPtr, 10); // NOLINT
          if (*endPtr == '\0') {
            return static_cast<uint64_t>(val);
          }
          return std::nullopt;
        }
        std::optional<Str> loadString(const char* key) const override {
          std::optional<String> str = _load(key, "string");
          if (!str.has_value())
            return std::nullopt;
          return Str(str->c_str());
        }

      private:
        FS* _fs = nullptr;
        std::string _root;

        bool _store(const char* key, const char* type, const char* value) {
          if (!_fs)
            return false;

          // ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "store(%s, %s, %s)", (_root + key).c_str(), type, value);

          File file = _fs->open((_root + key).c_str(), FILE_WRITE, true);
          if (!file) {
            // ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to open file for writing: %s", (_root + key).c_str());
            return false;
          }

          size_t ltype = strlen(type);
          size_t lvalue = strlen(value);
          size_t written = file.write((const uint8_t*)type, ltype);
          written += file.write(':');
          written += file.write((const uint8_t*)value, lvalue);
          file.close();

          return written == ltype + 1 + lvalue;
        }

        std::optional<String> _load(const char* key, const char* expectedType) const {
          // ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "load(%s, %s)", (_root + key).c_str(), expectedType);

          if (!_fs)
            return std::nullopt;

          std::string path = _root + key;
          if (!_fs->exists(path.c_str()))
            return std::nullopt;

          File file = _fs->open(path.c_str(), FILE_READ);
          if (!file)
            return std::nullopt;

          String content = file.readString();
          file.close();

          int sep = content.indexOf(':');
          if (sep < 0)
            return std::nullopt;

          String type = content.substring(0, sep);
          if (type != expectedType)
            return std::nullopt;

          // ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "load(%s, %s) => %s", (_root + key).c_str(), expectedType, content.substring(sep + 1).c_str());
          return content.substring(sep + 1);
        }
    };
  } // namespace config
} // namespace Mycila
