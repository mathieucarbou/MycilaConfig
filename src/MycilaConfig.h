// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <Print.h>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#include <esp32-hal-log.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#define MYCILA_CONFIG_VERSION          "10.0.0"
#define MYCILA_CONFIG_VERSION_MAJOR    10
#define MYCILA_CONFIG_VERSION_MINOR    0
#define MYCILA_CONFIG_VERSION_REVISION 0

#define MYCILA_CONFIG_LOG_TAG "CONFIG"

// suffix to use for a setting key enabling a feature
#ifndef MYCILA_CONFIG_KEY_ENABLE_SUFFIX
  #define MYCILA_CONFIG_KEY_ENABLE_SUFFIX "_enable"
#endif

// suffix to use for a setting key representing a password
#ifndef MYCILA_CONFIG_KEY_PASSWORD_SUFFIX
  #define MYCILA_CONFIG_KEY_PASSWORD_SUFFIX "_pwd"
#endif

#ifndef MYCILA_CONFIG_SHOW_PASSWORD
  #ifndef MYCILA_CONFIG_PASSWORD_MASK
    #define MYCILA_CONFIG_PASSWORD_MASK "********"
  #endif
#endif

#ifndef MYCILA_CONFIG_VALUE_TRUE
  #define MYCILA_CONFIG_VALUE_TRUE "true"
#endif

#ifndef MYCILA_CONFIG_VALUE_FALSE
  #define MYCILA_CONFIG_VALUE_FALSE "false"
#endif

#ifndef MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING
  #define MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING 1
#endif

namespace Mycila {
  class Config {
    public:
      /**
       * A string wrapper with a pointer that can point to flash memory or heap memory
       */
      class Str {
        public:
          Str() : Str("") {}
          Str(size_t length) { // NOLINT
            _buffer = new char[length + 1];
            _buffer[0] = '\0';
          }
          Str(const char* str); // NOLINT
          ~Str();
          Str(Str&& other) noexcept : _buffer(other._buffer) { other._buffer = nullptr; }
          Str& operator=(Str&& other) noexcept;
          Str(const Str& other);
          Str& operator=(const Str& other);
          bool inFlash() const;
          const char* c_str() const { return _buffer; }
          char* buffer() const { return _buffer; }
          size_t heapUsage() const;
          friend bool operator==(const Str& lhs, const Str& rhs) { return lhs._buffer == rhs._buffer || strcmp(lhs._buffer, rhs._buffer) == 0; }
          friend bool operator==(const Str& lhs, const char* rhs) { return lhs._buffer == rhs || strcmp(lhs._buffer, rhs) == 0; }
          friend bool operator==(const char* lhs, const Str& rhs) { return lhs == rhs._buffer || strcmp(lhs, rhs._buffer) == 0; }
          bool empty() const { return !_buffer || _buffer[0] == '\0'; }
          size_t length() const { return _buffer ? strlen(_buffer) : 0; }

        private:
          char* _buffer = nullptr;
      };

      /**
       * Abstract storage interface
       */
      class Storage {
        public:
          Storage() = default;
          virtual ~Storage() = default;

          virtual bool begin(const char* name) { return false; }
          virtual void end() {}

          virtual bool hasKey(const char* key) const { return false; }

          // returns true if the key was removed, or did not exist, false on failure
          virtual bool remove(const char* key) { return false; }
          virtual bool removeAll() { return false; }

          virtual bool storeBool(const char* key, bool value) { return false; }
          virtual bool storeFloat(const char* key, float value) { return false; }
          virtual bool storeDouble(const char* key, double value) { return false; }
          virtual bool storeI8(const char* key, int8_t value) { return false; }
          virtual bool storeU8(const char* key, uint8_t value) { return false; }
          virtual bool storeI16(const char* key, int16_t value) { return false; }
          virtual bool storeU16(const char* key, uint16_t value) { return false; }
          virtual bool storeI32(const char* key, int32_t value) { return false; }
          virtual bool storeU32(const char* key, uint32_t value) { return false; }
          virtual bool storeI64(const char* key, int64_t value) { return false; }
          virtual bool storeU64(const char* key, uint64_t value) { return false; }
          virtual bool storeString(const char* key, const char* value) { return false; }

          virtual std::optional<bool> loadBool(const char* key) const { return std::nullopt; }
          virtual std::optional<float> loadFloat(const char* key) const { return std::nullopt; }
          virtual std::optional<double> loadDouble(const char* key) const { return std::nullopt; }
          virtual std::optional<int8_t> loadI8(const char* key) const { return std::nullopt; }
          virtual std::optional<uint8_t> loadU8(const char* key) const { return std::nullopt; }
          virtual std::optional<int16_t> loadI16(const char* key) const { return std::nullopt; }
          virtual std::optional<uint16_t> loadU16(const char* key) const { return std::nullopt; }
          virtual std::optional<int32_t> loadI32(const char* key) const { return std::nullopt; }
          virtual std::optional<uint32_t> loadU32(const char* key) const { return std::nullopt; }
          virtual std::optional<int64_t> loadI64(const char* key) const { return std::nullopt; }
          virtual std::optional<uint64_t> loadU64(const char* key) const { return std::nullopt; }
          virtual std::optional<Str> loadString(const char* key) const { return std::nullopt; }
      };

      /**
       * Result status of a set/unset operation
       */
      enum class Status {
        PERSISTED,
        DEFAULTED,
        REMOVED,
        ERR_UNKNOWN_KEY,
        ERR_INVALID_TYPE,
        ERR_INVALID_VALUE,
        ERR_FAIL_ON_WRITE,
        ERR_FAIL_ON_REMOVE,
      };

      /**
       * Result of a set/unset operation
       */
      class Result {
        public:
          constexpr Result(Status status) noexcept : _status(status) {} // NOLINT
          // operation successful
          constexpr operator bool() const { return _status == Status::PERSISTED || _status == Status::DEFAULTED || _status == Status::REMOVED; }
          constexpr operator Status() const { return _status; }
          constexpr bool operator==(const Status& other) const { return _status == other; }
          // storage updated (value actually changed)
          constexpr bool isStorageUpdated() const { return _status == Status::PERSISTED || _status == Status::REMOVED; }

        private:
          Status _status;
      };

      /**
       * Value variant type
       */
      using Value = std::variant<
        std::monostate, // as null
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

      static const Value VOID;

      /**
       * Key definition
       */
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

      //////////////////
      // Callbacks //
      //////////////////

      typedef std::function<void(const char* key, const Value& newValue)> ChangeCallback;
      typedef std::function<void()> RestoredCallback;
      typedef std::function<bool(const char* key, const Value& newValue)> ValidatorCallback;

      //////////////////
      // Config class //
      //////////////////

      explicit Config(Storage& storage) : _storage(&storage) {}
      ~Config() = default;

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

      // starts the config system and returns true if successful
      bool begin(const char* name = "CONFIG", bool preload = false);

      // register a callback to be called when a config value changes
      void listen(ChangeCallback callback) { _changeCallback = std::move(callback); }

      // register a callback to be called when the configuration is restored
      void listen(RestoredCallback callback) { _restoreCallback = std::move(callback); }

      // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(ValidatorCallback callback);
      // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(const char* key, ValidatorCallback callback);

      // returns true if the key is configured
      bool configured(const char* key) const;
      // returns true if the key is stored
      bool stored(const char* key) const { return _storage->hasKey(key); }

      // get list of configured keys
      const std::vector<Key>& keys() const { return _keys; }
      // this method can be used to find the right pointer to a supported key given a random buffer
      const char* keyRef(const char* buffer) const;
      // this method can be used to find the right pointer to a supported key definition given a random buffer
      const Key* key(const char* buffer) const;

      template <typename T = Value>
      const Result set(const char* key, T value, bool fireChangeCallback = true) { return _set(key, Value(value), fireChangeCallback); }
      const Result setString(const char* key, const std::string& value, bool fireChangeCallback = true) { return setString(key, value.c_str(), fireChangeCallback); }
      // set the value of a setting key, and if the key is null, it will be unset
      const Result setString(const char* key, const char* value, bool fireChangeCallback = true) { return this->set<Str>(key, Str(value), fireChangeCallback); }
      bool set(std::map<const char*, Value> settings, bool fireChangeCallback = true);

      // returns the value of a setting key, or nullptr if the key is not configured
      template <typename T = Value>
      const T& get(const char* key) const {
        const auto& variant = _get(key);
        if constexpr (std::is_same_v<T, std::string> && std::holds_alternative<Str>(variant)) {
          return std::string{std::get<Str>(variant).c_str()};
        } else if (std::holds_alternative<T>(variant)) {
          return std::get<T>(variant);
        }
        throw std::runtime_error("Invalid type conversion");
      }
      const char* getString(const char* key) const { return this->get<Str>(key).c_str(); }

      Result unset(const char* key, bool fireChangeCallback = true);
      // clear cache and remove all stored keys and values
      void clear();

      bool isEmpty(const char* key) const { return getString(key)[0] == '\0'; }
      bool isEqual(const char* key, const std::string& value) const { return isEqual(key, value.c_str()); }
      bool isEqual(const char* key, const char* value) const {
        const char* v = getString(key);
        return v == value || strcmp(v, value) == 0;
      }
      bool isPasswordKey(const char* key) const;
      bool isEnableKey(const char* key) const;

      // backup current settings to a Print stream
      // if includeDefaults is true, also include default values for keys not set by the user
      void backup(Print& out, bool includeDefaults = true); // NOLINT
      bool restore(const char* data);
      bool restore(std::map<const char*, Value> settings);

      // approximate heap usage by config system (keys, defaults, cached values)
      size_t heapUsage() const;

#ifdef MYCILA_JSON_SUPPORT
      void toJson(const JsonObject& root);
#endif

      static std::string toString(const Value& value);
      static std::optional<Value> fromString(const char* str, const Value& defaultValue);

    private:
      Storage* _storage;
      ChangeCallback _changeCallback = nullptr;
      RestoredCallback _restoreCallback = nullptr;
      ValidatorCallback _globalValidatorCallback = nullptr;
      std::vector<Key> _keys;
      mutable std::map<const char*, Value> _cache;
      mutable std::map<const char*, ValidatorCallback> _validators;

      const Value& _get(const char* key) const;
      const Result _set(const char* key, Value value, bool fireChangeCallback = true);
  };
} // namespace Mycila
