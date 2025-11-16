// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <Print.h>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#define MYCILA_CONFIG_VERSION          "9.0.3"
#define MYCILA_CONFIG_VERSION_MAJOR    9
#define MYCILA_CONFIG_VERSION_MINOR    0
#define MYCILA_CONFIG_VERSION_REVISION 3

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
  typedef std::function<void(const char* key, const char* newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;
  typedef std::function<bool(const char* key, const char* newValue)> ConfigValidatorCallback;

  class Config {
    public:
      /**
       * A string wrapper with a pointer that can point to flash memory or heap memory
       */
      class Str {
        public:
          Str() : Str("") {}
          explicit Str(size_t length);
          explicit Str(const char* str);
          ~Str();
          Str(Str&& other) noexcept;
          Str& operator=(Str&& other) noexcept;
          Str(const Str& other) = delete;
          Str& operator=(const Str& other) = delete;
          bool inFlash() const;
          const char* c_str() const { return _buffer; }
          char* buffer() const { return _buffer; }
          size_t heapUsage() const;

        private:
          char* _buffer;
      };

      /**
       * Abstract storage interface
       */
      class Storage {
        public:
          Storage() = default;
          virtual ~Storage() = default;

          virtual bool begin(const char* name) { return false; }

          virtual bool hasKey(const char* key) const { return false; }

          // returns true if the key was removed, or did not exist, false on failure
          virtual bool remove(const char* key) { return false; }
          virtual bool removeAll() { return false; }

          virtual bool storeBool(const char* key, bool value) { return false; }
          virtual bool storeFloat(const char* key, float_t value) { return false; }
          virtual bool storeDouble(const char* key, double_t value) { return false; }
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
          virtual std::optional<float_t> loadFloat(const char* key) const { return std::nullopt; }
          virtual std::optional<double_t> loadDouble(const char* key) const { return std::nullopt; }
          virtual std::optional<int8_t> loadI8(const char* key) const { return std::nullopt; }
          virtual std::optional<uint8_t> loadU8(const char* key) const { return std::nullopt; }
          virtual std::optional<int16_t> loadI16(const char* key) const { return std::nullopt; }
          virtual std::optional<uint16_t> loadU16(const char* key) const { return std::nullopt; }
          virtual std::optional<int32_t> loadI32(const char* key) const { return std::nullopt; }
          virtual std::optional<uint32_t> loadU32(const char* key) const { return std::nullopt; }
          virtual std::optional<int64_t> loadI64(const char* key) const { return std::nullopt; }
          virtual std::optional<uint64_t> loadU64(const char* key) const { return std::nullopt; }
          virtual std::optional<Str> load(const char* key) const { return std::nullopt; }
      };

      /**
       * Result status of a set/unset operation
       */
      enum class Status {
        PERSISTED,
        DEFAULTED,
        REMOVED,
        ERR_UNKNOWN_KEY,
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

      explicit Config(Storage& storage) : _storage(&storage) {}
      ~Config() = default;

      // Add a new configuration key with its default value and optional validator
      // Returns true if the key was added, false otherwise (e.g. key too long)
      bool configure(const char* key, const std::string& defaultValue, ConfigValidatorCallback callback = nullptr) { return configure(key, defaultValue.c_str(), callback); }
      bool configure(const char* key, const char* defaultValue = "", ConfigValidatorCallback callback = nullptr);

      // starts the config system and returns true if successful
      bool begin(const char* name = "CONFIG");

      // register a callback to be called when a config value changes
      void listen(ConfigChangeCallback callback) { _changeCallback = std::move(callback); }

      // register a callback to be called when the configuration is restored
      void listen(ConfigRestoredCallback callback) { _restoreCallback = std::move(callback); }

      // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(ConfigValidatorCallback callback);
      // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(const char* key, ConfigValidatorCallback callback);

      // returns true if the key is configured
      bool configured(const char* key) const;
      // returns true if the key is stored
      bool stored(const char* key) const { return _storage->hasKey(key); }

      // get list of configured keys
      const std::vector<const char*>& keys() const { return _keys; }
      // this method can be used to find the right pointer to a supported key given a random buffer
      const char* keyRef(const char* buffer) const;

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
      const Result set(const char* key, const std::string& value, bool fireChangeCallback = true) { return set(key, value.c_str(), fireChangeCallback); }
      // set the value of a setting key, and if the key is null, it will be unset
      const Result set(const char* key, const char* value, bool fireChangeCallback = true);

      bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true);

      bool getBool(const char* key) const;
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
      std::string getString(const char* key) const { return std::string(get(key)); }
      // returns the value of a setting key, or nullptr if the key is not configured
      const char* get(const char* key) const;

      Result unset(const char* key, bool fireChangeCallback = true);
      // clear cache and remove all stored keys and values
      void clear();

      bool isEmpty(const char* key) const { return get(key)[0] == '\0'; }
      bool isEqual(const char* key, const std::string& value) const { return isEqual(key, value.c_str()); }
      bool isEqual(const char* key, const char* value) const { return strcmp(get(key), value) == 0; }
      bool isPasswordKey(const char* key) const;
      bool isEnableKey(const char* key) const;

      // backup current settings to a Print stream
      // if includeDefaults is true, also include default values for keys not set by the user
      void backup(Print& out, bool includeDefaults = true); // NOLINT
      bool restore(const char* data);
      bool restore(const std::map<const char*, std::string>& settings);

      // approximate heap usage by config system (keys, defaults, cached values)
      size_t heapUsage() const;

#ifdef MYCILA_JSON_SUPPORT
      void toJson(const JsonObject& root);
#endif

    private:
      Storage* _storage;
      ConfigChangeCallback _changeCallback = nullptr;
      ConfigRestoredCallback _restoreCallback = nullptr;
      ConfigValidatorCallback _globalValidatorCallback = nullptr;
      std::vector<const char*> _keys;
      mutable std::map<const char*, Str> _defaults;
      mutable std::map<const char*, Str> _cache;
      mutable std::map<const char*, ConfigValidatorCallback> _validators;
  };
} // namespace Mycila
