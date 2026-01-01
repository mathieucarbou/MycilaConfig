// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../config/Config.h"

namespace Mycila {
  namespace config {
    class ConfigV10 {
      public:
        explicit ConfigV10(Config& delegate) : _delegate(&delegate) {}
        ~ConfigV10() = default;

        bool configure(const char* key, const std::string& defaultValue, ValidatorCallback callback = nullptr) { return configure(key, defaultValue.c_str(), callback); }
        bool configure(const char* key, const char* defaultValue = "", ValidatorCallback callback = nullptr) { return _delegate->configure<Value>(key, Value(defaultValue), std::move(callback)); }

        bool begin(const char* name = "CONFIG", bool preload = false) { return _delegate->begin(name, preload); }

        void listen(ChangeCallback callback) { _delegate->listen(std::move(callback)); }
        void listen(RestoredCallback callback) { _delegate->listen(std::move(callback)); }

        bool setValidator(ValidatorCallback callback) { return _delegate->setValidator(std::move(callback)); }
        bool setValidator(const char* key, ValidatorCallback callback) { return _delegate->setValidator(key, std::move(callback)); }

        bool configured(const char* key) const { return _delegate->configured(key); }

        bool stored(const char* key) const { return _delegate->stored(key); }

        const std::vector<Key>& keys() const { return _delegate->keys(); }
        const char* keyRef(const char* buffer) const { return _delegate->keyRef(buffer); }
        const Key* key(const char* buffer) const { return _delegate->key(buffer); }

        bool setBool(const char* key, bool value, bool fireChangeCallback = true) { return set(key, value ? MYCILA_CONFIG_VALUE_TRUE : MYCILA_CONFIG_VALUE_FALSE, fireChangeCallback); }
        bool setFloat(const char* key, float_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setDouble(const char* key, double_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setInt(const char* key, int value, bool fireChangeCallback = true) { return setI32(key, value, fireChangeCallback); }
        bool setLong(const char* key, long value, bool fireChangeCallback = true) { return setI32(key, value, fireChangeCallback); } // NOLINT
        bool setI8(const char* key, int8_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setU8(const char* key, uint8_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setI16(const char* key, int16_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setU16(const char* key, uint16_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setI32(const char* key, int32_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setU32(const char* key, uint32_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setI64(const char* key, int64_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        bool setU64(const char* key, uint64_t value, bool fireChangeCallback = true) { return set(key, std::to_string(value), fireChangeCallback); }
        const Result setString(const char* key, const std::string& value, bool fireChangeCallback = true) { return set(key, value.c_str(), fireChangeCallback); }
        const Result setString(const char* key, const char* value, bool fireChangeCallback = true) { return set(key, value, fireChangeCallback); }
        const Result set(const char* key, const std::string& value, bool fireChangeCallback = true) { return set(key, value.c_str(), fireChangeCallback); }
        const Result set(const char* key, const char* value, bool fireChangeCallback = true) {
          if (value == nullptr) {
            return _delegate->unset(key, fireChangeCallback);
          } else {
            return _delegate->setString(key, value, fireChangeCallback);
          }
        }

        bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true) {
          std::map<const char*, Value> vals;
          for (const auto& [key, value] : settings) {
            vals[key] = Value(value.c_str());
          }
          return _delegate->set(std::move(vals), fireChangeCallback);
        }

        bool getBool(const char* key) const {
          std::string val = get(key);
          if (val == MYCILA_CONFIG_VALUE_TRUE) {
            return true;
          }
#if MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING
          if (val == "true" || val == "1" || val == "on" || val == "yes") {
            return true;
          }
#endif
          return false;
        }
        float getFloat(const char* key) const { return std::stof(get(key)); }
        double getDouble(const char* key) const { return std::stod(get(key)); }
        int getInt(const char* key) const { return getI32(key); }
        long getLong(const char* key) const { return getI32(key); } // NOLINT
        int8_t getI8(const char* key) const { return static_cast<int8_t>(std::stoi(get(key))); }
        uint8_t getU8(const char* key) const { return static_cast<uint8_t>(std::stoul(get(key))); }
        int16_t getI16(const char* key) const { return static_cast<int16_t>(std::stoi(get(key))); }
        uint16_t getU16(const char* key) const { return static_cast<uint16_t>(std::stoul(get(key))); }
        int32_t getI32(const char* key) const { return static_cast<int32_t>(std::stol(get(key))); }
        uint32_t getU32(const char* key) const { return static_cast<uint32_t>(std::stoul(get(key))); }
        int64_t getI64(const char* key) const { return static_cast<int64_t>(std::stoll(get(key))); }
        uint64_t getU64(const char* key) const { return static_cast<uint64_t>(std::stoull(get(key))); }
        const char* getString(const char* key) const { return get(key); }
        const char* get(const char* key) const { return _delegate->getString(key); }

        Result unset(const char* key, bool fireChangeCallback = true) { return _delegate->unset(key, fireChangeCallback); }

        void clear() { _delegate->clear(); }

        bool isEmpty(const char* key) const { return _delegate->isEmpty(key); }
        bool isEqual(const char* key, const std::string& value) const { return _delegate->isEqual(key, value); }
        bool isEqual(const char* key, const char* value) const { return _delegate->isEqual(key, value); }
        bool isPasswordKey(const char* key) const { return this->key(key)->isPasswordKey(); }
        bool isEnableKey(const char* key) const { return this->key(key)->isEnableKey(); }

        void backup(Print& out, bool includeDefaults = true) { _delegate->backup(out, includeDefaults); }
        bool restore(const char* data) { return _delegate->restore(data); }
        bool restore(const std::map<const char*, std::string>& settings) { return _delegate->restore(std::move(settings)); }

        size_t heapUsage() const { return _delegate->heapUsage(); }

#ifdef MYCILA_JSON_SUPPORT
        void toJson(const JsonObject& root) { _delegate->toJson(root); }
#endif

      private:
        Config* _delegate;
    };
  } // namespace config
} // namespace Mycila
