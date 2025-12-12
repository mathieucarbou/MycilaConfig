// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <esp32-hal-log.h>

#include <Print.h>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "./Defines.h"
#include "./Key.h"
#include "./Result.h"
#include "./Storage.h"
#include "./Str.h"
#include "./Value.h"

namespace Mycila {
  namespace config {
    typedef std::function<void(const char* key, const Value& newValue)> ChangeCallback;
    typedef std::function<void()> RestoredCallback;
    typedef std::function<bool(const char* key, const Value& newValue)> ValidatorCallback;

    class Config {
      public:
        explicit Config(Storage& storage) : _storage(&storage) {}
        ~Config() { end(); }

        // Add a new configuration key with its default value and optional validator
        // Returns true if the key was added, false otherwise (e.g. key too long)
        template <typename T = Value>
        bool configure(const char* key, T defaultValue, ValidatorCallback callback = nullptr) {
          _keys.push_back(Key(key, std::move(defaultValue)));
          std::sort(_keys.begin(), _keys.end(), [](const Key& a, const Key& b) { return strcmp(a.name, b.name) < 0; });
          ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "configure(%s)", key);
          if (callback) {
            _validators[key] = std::move(callback);
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, callback)", key);
          }
          return true;
        }
        bool configure(const char* key, ValidatorCallback callback = nullptr) {
          return configure<Value>(key, Value(""), std::move(callback));
        }
        bool configure(const char* key, std::string defaultValue, ValidatorCallback callback = nullptr) {
          return configure<Value>(key, Value(defaultValue.c_str()), std::move(callback));
        }

        // starts the config system and returns true if successful
        bool begin(const char* name, bool preload = false) {
          ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Initializing Config '%s'", name);
          if (!_storage->begin(name)) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to initialize storage backend!");
            return false;
          }
          if (preload) {
            ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Preloading Config '%s'", name);
            for (auto& key : _keys) {
              auto value = _storage->loadString(key.name);
              if (value.has_value()) {
                _cache[key.name] = std::move(value.value());
                ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "get(%s): CACHED", key.name);
              }
            }
          }
          _name = name;
          return true;
        }

        void end() {
          _name = nullptr;
          _storage->end();
          _cache.clear();
        }

        Storage& storage() const { return *_storage; }

        const char* name() const { return _name; }

        bool enabled() const { return _name != nullptr; }

        // register a callback to be called when a config value changes
        void listen(ChangeCallback callback) { _changeCallback = std::move(callback); }

        // register a callback to be called when the configuration is restored
        void listen(RestoredCallback callback) { _restoreCallback = std::move(callback); }

        // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
        bool setValidator(ValidatorCallback callback) {
          if (callback) {
            _globalValidatorCallback = std::move(callback);
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(callback)");
          } else {
            _globalValidatorCallback = nullptr;
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(nullptr)");
          }
          return true;
        }

        // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
        bool setValidator(const char* key, ValidatorCallback callback) {
          Key* k = const_cast<Key*>(this->key(key));

          // check if the key is valid
          if (k == nullptr) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "setValidator(%s): Unknown key!", key);
            return false;
          }

          if (callback) {
            _validators[key] = std::move(callback);
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, callback)", key);
          } else {
            _validators.erase(key);
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, nullptr)", key);
          }

          return true;
        }

        // returns true if the key is configured
        bool configured(const char* key) const { return this->key(key) != nullptr; }

        // returns true if the key is stored
        bool stored(const char* key) const { return enabled() && _storage->hasKey(key); }

        // get list of configured keys
        const std::vector<Key>& keys() const { return _keys; }

        // this method can be used to find the right pointer to a supported key given a random buffer
        const char* keyRef(const char* buffer) const {
          const Key* k = this->key(buffer);
          return k ? k->name : nullptr;
        }

        // this method can be used to find the right pointer to a supported key definition given a random buffer
        const Key* key(const char* buffer) const {
          auto it = std::lower_bound(_keys.begin(), _keys.end(), buffer, [](const Key& a, const char* b) { return strcmp(a.name, b) < 0; });
          return it != _keys.end() && (it->name == buffer || strcmp(it->name, buffer) == 0) ? &(*it) : nullptr;
        }

        template <typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, Value>, int> = 0>
        const Result set(const char* key, T value, bool fireChangeCallback = true) { return _set(key, Value(value), fireChangeCallback); }
        const Result set(const char* key, Value value, bool fireChangeCallback = true) { return _set(key, std::move(value), fireChangeCallback); }

        const Result setString(const char* key, const std::string& value, bool fireChangeCallback = true) { return setString(key, value.c_str(), fireChangeCallback); }

        // set the value of a setting key, and if the key is null, it will be unset
        const Result setString(const char* key, const char* value, bool fireChangeCallback = true) { return this->set<Str>(key, Str(value), fireChangeCallback); }

        bool set(std::map<const char*, Value> settings, bool fireChangeCallback = true) {
          bool updates = false;
          // start restoring settings
          for (const auto& k : _keys)
            if (!k.isEnableKey() && settings.find(k.name) != settings.end())
              updates |= _set(k.name, std::move(settings.at(k.name)), fireChangeCallback).isStorageUpdated();
          // then restore settings enabling/disabling a feature
          for (const auto& k : _keys)
            if (k.isEnableKey() && settings.find(k.name) != settings.end())
              updates |= _set(k.name, std::move(settings.at(k.name)), fireChangeCallback).isStorageUpdated();
          return updates;
        }

        bool set(std::map<const char*, std::string> settings, bool fireChangeCallback = true) { return set(_convert(settings), fireChangeCallback); }

        // returns the value of a setting key or its default value if not set
        template <typename T = Value>
        auto get(const char* key) const {
          const Key* k = this->key(key);

          // check if key is configured
          if (k == nullptr) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "get(%s): ERR_UNKNOWN_KEY", key);
            throw std::runtime_error("Unknown key");
          }

          return _get(*k).as<T>();
        }

        const char* getString(const char* key) const { return this->get<const char*>(key); }

        Result unset(const char* key, bool fireChangeCallback = true) {
          if (!enabled()) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "unset(%s): ERR_DISABLED", key);
            return Status::ERR_DISABLED;
          }

          const Key* k = this->key(key);

          // check if the key is valid
          if (k == nullptr) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "unset(%s): ERR_UNKNOWN_KEY", key);
            return Status::ERR_UNKNOWN_KEY;
          }

          // or not removed
          if (!_storage->remove(key)) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "unset(%s): ERR_FAIL_ON_REMOVE", key);
            return Status::ERR_FAIL_ON_REMOVE;
          }

          // key there and to remove
          _cache.erase(key);
          ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "unset(%s): REMOVED", key);
          if (fireChangeCallback && _changeCallback)
            _changeCallback(key, k->defaultValue);

          return Status::REMOVED;
        }

        // clear cache and remove all stored keys and values
        void clear() {
          _storage->removeAll();
          _cache.clear();
        }

        bool isEmpty(const char* key) const { return getString(key)[0] == '\0'; }

        bool isEqual(const char* key, const std::string& value) const { return isEqual(key, value.c_str()); }

        bool isEqual(const char* key, const char* value) const {
          const char* v = getString(key);
          return v == value || strcmp(v, value) == 0;
        }

        // backup current settings to a Print stream
        // if includeDefaults is true, also include default values for keys not set by the user
        void backup(Print& out, bool includeDefaults = true) { // NOLINT
          for (auto& key : _keys) {
            if (includeDefaults || stored(key.name)) {
              const Value& v = _get(key);
              out.print(key.name);
              out.print('=');
              out.print(v.toString().c_str());
              out.print("\n");
            }
          }
        }

        bool restore(const char* data) {
          std::map<const char*, Value> settings;
          for (auto& key : _keys) {
            // int start = data.indexOf(key);
            char* start = strstr(data, key.name);
            if (start) {
              start += strlen(key.name);
              if (*start != '=')
                continue;
              start++;
              char* end = strchr(start, '\r');
              if (!end)
                end = strchr(start, '\n');
              if (!end) {
                ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "restore(%s): Invalid data!", key.name);
                return false;
              }
              std::optional<Value> value = Value::fromString(std::string(start, end - start).c_str(), key.defaultValue);
              if (value.has_value()) {
                settings[key.name] = std::move(value.value());
              } else {
                ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "restore(%s): Invalid data!", key.name);
                return false;
              }
            }
          }
          return restore(std::move(settings));
        }

        bool restore(std::map<const char*, Value> settings) {
          ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "Restoring %d settings", settings.size());
          bool restored = set(std::move(settings), false);
          if (restored) {
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "Config restored");
            if (_restoreCallback)
              _restoreCallback();
          } else
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "No change detected");
          return restored;
        }
        bool restore(std::map<const char*, std::string> settings) { return restore(_convert(settings)); }

        // approximate heap usage by config system (keys, defaults, cached values)
        size_t heapUsage() const {
          size_t total = 0;

          // Red-black tree node overhead (without the payload)
          static constexpr size_t rbTreeNodeOverhead = 3 * sizeof(void*) + sizeof(char);

          // variant size
          static constexpr size_t variantSize = sizeof(Value);

          // _keys vector: the vector itself has capacity-based allocation
          // Each Key contains: const char* name (pointer to flash) + Str defaultValue
          if (_keys.capacity() > 0) {
            total += _keys.capacity() * sizeof(Key);

            // Check each Key's defaultValue for heap-allocated strings
            for (const auto& key : _keys) {
              total += variantSize;
              total += std::holds_alternative<Str>(key.defaultValue) ? key.defaultValue.as<Str>().heapUsage() : 0;
            }
          }

          // cache map: std::map<const char*, Str>
          // Each node: rb-tree overhead + pair<const char*, Str>
          static constexpr size_t cachePairSize = sizeof(const char*) + sizeof(Str);
          static constexpr size_t cacheNodeSize = rbTreeNodeOverhead + cachePairSize;

          for (const auto& [key, val] : _cache) {
            total += cacheNodeSize; // map node + pair structure
            total += variantSize;
            total += std::holds_alternative<Str>(val) ? val.as<Str>().heapUsage() : 0;
          }

          // validators map: std::map<const char*, ValidatorCallback>
          // Each node: rb-tree overhead + pair<const char*, ValidatorCallback>
          static constexpr size_t validatorPairSize = sizeof(const char*) + sizeof(ValidatorCallback);
          total += _validators.size() * (rbTreeNodeOverhead + validatorPairSize);

          return total;
        }

#ifdef MYCILA_JSON_SUPPORT
        void toJson(const JsonObject& root) {
          for (auto& key : _keys) {
            toJson(key, root[key.name].to<JsonVariant>());
          }
        }

        void toJson(const Key& key, const JsonVariant& out) {
          std::visit(
            [&](auto&& value) -> void {
              using T = std::decay_t<decltype(value)>;
              if constexpr (std::is_same_v<T, bool> || std::is_arithmetic_v<T>) {
                out.set(value);
              } else if constexpr (std::is_same_v<T, Str>) {
  #ifdef MYCILA_CONFIG_PASSWORD_MASK
                out.set(!key.isPasswordKey() ? value.c_str() : MYCILA_CONFIG_PASSWORD_MASK);
  #else
                out.set(value.c_str());
  #endif
              }
            },
            _get(key));
        }
#endif

      private:
        friend class Migration;

        Storage* _storage;
        const char* _name = nullptr;
        ChangeCallback _changeCallback = nullptr;
        RestoredCallback _restoreCallback = nullptr;
        ValidatorCallback _globalValidatorCallback = nullptr;
        std::vector<Key> _keys;
        mutable std::map<const char*, Value> _cache;
        mutable std::map<const char*, ValidatorCallback> _validators;

        std::map<const char*, Value> _convert(const std::map<const char*, std::string>& settings) {
          std::map<const char*, Value> converted;
          for (const auto& [k, f] : settings) {
            const Key* key = this->key(k);
            if (key == nullptr) {
              ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "convert(): Unknown key '%s'", k);
              continue;
            }
            std::optional<Value> opt = Value::fromString(f.c_str(), key->defaultValue);
            if (opt.has_value()) {
              converted[key->name] = std::move(opt.value());
            } else {
              ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "convert(): Invalid value for key '%s': '%s'", key->name, f.c_str());
            }
          }
          return converted;
        }

        const Value& _get(const Key& key) const {
          if (!enabled()) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "get(%s): ERR_DISABLED", key.name);
            return key.defaultValue;
          }

          // check if we have a cached value
          auto it = _cache.find(key.name);
          if (it != _cache.end()) {
            ESP_LOGV(MYCILA_CONFIG_LOG_TAG, "get(%s): CACHE HIT", key.name);
            return it->second;
          }

          std::optional<Value> value = std::visit(
            [&](auto&& def) -> std::optional<Value> {
              using T = std::decay_t<decltype(def)>;
              return _load<T>(key.name);
            },
            key.defaultValue);

          // real key exists ?
          if (value.has_value()) {
            // allocate and copy the string to cache
            _cache[key.name] = std::move(value.value());
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "get(%s): CACHED", key.name);
            return _cache[key.name];
          }

          // key does not exist, or not assigned to a value
          ESP_LOGV(MYCILA_CONFIG_LOG_TAG, "get(%s): DEFAULT", key.name);
          return key.defaultValue;
        }

        const Result _set(const char* key, Value value, bool fireChangeCallback = true) {
          if (!enabled()) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_DISABLED", key);
            return Status::ERR_DISABLED;
          }

          const Key* k = this->key(key);

          // check if the key is valid
          if (k == nullptr) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_UNKNOWN_KEY", key);
            return Status::ERR_UNKNOWN_KEY;
          }

          // check if the type valid
          if (value.index() != k->defaultValue.index()) {
            ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_INVALID_TYPE", key);
            return Status::ERR_INVALID_TYPE;
          }

          const bool keyPersisted = stored(key);

          // key not there and set to default value
          if (!keyPersisted && k->defaultValue == value) {
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): DEFAULTED", key);
            return Status::DEFAULTED;
          }

          // check if we have a global validator
          // and check if the value is valid
          if (_globalValidatorCallback && !_globalValidatorCallback(key, value)) {
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_INVALID_VALUE", key);
            return Status::ERR_INVALID_VALUE;
          }

          // check if we have a specific validator for the key
          auto it = _validators.find(k->name);
          if (it != _validators.end()) {
            // check if the value is valid
            if (!it->second(k->name, value)) {
              ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_INVALID_VALUE", key);
              return Status::ERR_INVALID_VALUE;
            }
          }

          // update failed ?
          if (_store(key, value)) {
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): PERSISTED", key);
          } else {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_FAIL_ON_WRITE", key);
            return Status::ERR_FAIL_ON_WRITE;
          }

          _cache[key] = std::move(value);
          ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): CACHED", key);

          if (fireChangeCallback && _changeCallback) {
            // NOTE: The 'value' pointer passed to the callback is only valid during this callback execution.
            // Do NOT store or use the pointer after the callback returns.
            _changeCallback(key, _cache[key]);
          }

          return Status::PERSISTED;
        }

        bool _store(const char* key, const Value& value) {
          return std::visit(
            [&](auto&& arg) -> bool {
              using T = std::decay_t<decltype(arg)>;
              if constexpr (std::is_same_v<T, bool>) {
                return _storage->storeBool(key, arg);
              } else if constexpr (std::is_same_v<T, int8_t>) {
                return _storage->storeI8(key, arg);
              } else if constexpr (std::is_same_v<T, uint8_t>) {
                return _storage->storeU8(key, arg);
              } else if constexpr (std::is_same_v<T, int16_t>) {
                return _storage->storeI16(key, arg);
              } else if constexpr (std::is_same_v<T, uint16_t>) {
                return _storage->storeU16(key, arg);
              } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
                return _storage->storeI32(key, arg);
              } else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
                return _storage->storeU32(key, arg);
#if MYCILA_CONFIG_USE_LONG_LONG
              } else if constexpr (std::is_same_v<T, int64_t>) {
                return _storage->storeI64(key, arg);
              } else if constexpr (std::is_same_v<T, uint64_t>) {
                return _storage->storeU64(key, arg);
#endif
              } else if constexpr (std::is_same_v<T, float>) {
                return _storage->storeFloat(key, arg);
#if MYCILA_CONFIG_USE_DOUBLE
              } else if constexpr (std::is_same_v<T, double>) {
                return _storage->storeDouble(key, arg);
#endif
              } else if constexpr (std::is_same_v<T, Str>) {
                return _storage->storeString(key, arg.c_str());
              }
              return false;
            },
            value);
        }

        template <typename T = Value>
        std::optional<T> _load(const char* key) const {
          if constexpr (std::is_same_v<T, bool>)
            return _storage->loadBool(key);
          else if constexpr (std::is_same_v<T, int8_t>)
            return _storage->loadI8(key);
          else if constexpr (std::is_same_v<T, uint8_t>)
            return _storage->loadU8(key);
          else if constexpr (std::is_same_v<T, int16_t>)
            return _storage->loadI16(key);
          else if constexpr (std::is_same_v<T, uint16_t>)
            return _storage->loadU16(key);
          else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>)
            return _storage->loadI32(key);
          else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>)
            return _storage->loadU32(key);
#if MYCILA_CONFIG_USE_LONG_LONG
          else if constexpr (std::is_same_v<T, int64_t>)
            return _storage->loadI64(key);
          else if constexpr (std::is_same_v<T, uint64_t>)
            return _storage->loadU64(key);
#endif
          else if constexpr (std::is_same_v<T, float>)
            return _storage->loadFloat(key);
#if MYCILA_CONFIG_USE_DOUBLE
          else if constexpr (std::is_same_v<T, double>)
            return _storage->loadDouble(key);
#endif
          else if constexpr (std::is_same_v<T, Str>)
            return _storage->loadString(key);
          return std::nullopt;
        }
    };
  } // namespace config
} // namespace Mycila
