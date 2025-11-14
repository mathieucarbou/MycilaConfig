// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <Preferences.h>
#include <Print.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#define MYCILA_CONFIG_VERSION          "9.0.1"
#define MYCILA_CONFIG_VERSION_MAJOR    9
#define MYCILA_CONFIG_VERSION_MINOR    0
#define MYCILA_CONFIG_VERSION_REVISION 1

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
      enum class Status {
        PERSISTED,
        PERSISTED_ALREADY,
        PERSISTED_AS_DEFAULT,
        REMOVED,
        REMOVED_ALREADY,
        ERR_UNKNOWN_KEY,
        ERR_INVALID_VALUE,
        ERR_FAIL_ON_WRITE,
        ERR_FAIL_ON_REMOVE,
      };

      class Result {
        public:
          constexpr Result(Status status) noexcept : _status(status) {} // NOLINT
          // operation successful
          constexpr operator bool() const { return _status == Status::PERSISTED || _status == Status::PERSISTED_ALREADY || _status == Status::PERSISTED_AS_DEFAULT || _status == Status::REMOVED || _status == Status::REMOVED_ALREADY; }
          constexpr operator Status() const { return _status; }
          constexpr bool operator==(const Status& other) const { return _status == other; }
          // storage updated (value actually changed)
          constexpr bool isStorageUpdated() const { return _status == Status::PERSISTED || _status == Status::REMOVED; }

        private:
          Status _status;
      };

      ~Config();

      // Add a new configuration key with its default value
      // Returns true if the key was added, false otherwise (e.g. key too long)
      bool configure(const char* key, const std::string& defaultValue) { return configure(key, defaultValue.c_str()); }
      bool configure(const char* key, const char* defaultValue = "");

      // starts the config system
      void begin(const char* name = "CONFIG");

      // register a callback to be called when a config value changes
      void listen(ConfigChangeCallback callback) { _changeCallback = std::move(callback); }

      // register a callback to be called when the configuration is restored
      void listen(ConfigRestoredCallback callback) { _restoreCallback = std::move(callback); }

      // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(ConfigValidatorCallback callback);

      // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(const char* key, ConfigValidatorCallback callback);

      // returns true if the key is configured
      bool exists(const char* key) const { return std::find(_keys.begin(), _keys.end(), key) != _keys.end(); };
      // returns true if the key is stored
      bool stored(const char* key) const { return _prefs.isKey(key); }

      // get the value of a setting key
      // returns nullptr if the key is not supported (not configured)
      const char* get(const char* key) const;
      std::string getString(const char* key) const { return std::string(get(key)); }
      bool getBool(const char* key) const;
      long getLong(const char* key) const { return std::stol(get(key)); } // NOLINT
      int getInt(const char* key) const { return std::stoi(get(key)); }   // NOLINT
      float getFloat(const char* key) const { return std::stof(get(key)); }
      bool isEmpty(const char* key) const { return get(key)[0] == '\0'; }
      bool isEqual(const char* key, const std::string& value) const { return value == get(key); }
      bool isEqual(const char* key, const char* value) const { return strcmp(get(key), value) == 0; }

      const Result set(const char* key, const std::string& value, bool fireChangeCallback = true) { return set(key, value.c_str(), fireChangeCallback); }
      // set the value of a setting key
      // if the key is null, it will be unset
      const Result set(const char* key, const char* value, bool fireChangeCallback = true);
      bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true);
      bool setBool(const char* key, bool value) { return set(key, value ? MYCILA_CONFIG_VALUE_TRUE : MYCILA_CONFIG_VALUE_FALSE); }

      Result unset(const char* key, bool fireChangeCallback = true);

      bool isPasswordKey(const char* key) const;
      bool isEnableKey(const char* key) const;

      // backup current settings to a Print stream
      // if includeDefaults is true, also include default values for keys not set by the user
      void backup(Print& out, bool includeDefaults = true); // NOLINT
      bool restore(const char* data);
      bool restore(const std::map<const char*, std::string>& settings);

      // clear all saved settings and current cache
      void clear();

      // get list of keys
      const std::vector<const char*>& keys() const { return _keys; }

      // this method can be used to find the right pointer to a supported key given a random buffer
      const char* keyRef(const char* buffer) const;

#ifdef MYCILA_JSON_SUPPORT
      void toJson(const JsonObject& root);
#endif

    private:
      ConfigChangeCallback _changeCallback = nullptr;
      ConfigRestoredCallback _restoreCallback = nullptr;
      ConfigValidatorCallback _globalValidatorCallback = nullptr;
      std::vector<const char*> _keys;
      mutable Preferences _prefs;
      mutable std::map<const char*, std::unique_ptr<char[], void(*)(char[])>> _defaults;
      mutable std::map<const char*, std::unique_ptr<char[], void(*)(char[])>> _cache;
      mutable std::map<const char*, ConfigValidatorCallback> _validators;
  };
} // namespace Mycila
