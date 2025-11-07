// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <Preferences.h>
#include <Print.h>
#include <map>
#include <string>
#include <vector>

#ifdef MYCILA_CONFIG_SUPPORT_LONG_KEYS
  #include "mbedtls/md.h"
#endif

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#define MYCILA_CONFIG_VERSION          "7.3.0"
#define MYCILA_CONFIG_VERSION_MAJOR    7
#define MYCILA_CONFIG_VERSION_MINOR    3
#define MYCILA_CONFIG_VERSION_REVISION 0

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
  typedef std::function<void(const char* key, const std::string& newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;
  typedef std::function<bool(const char* key, const std::string& newValue)> ConfigValidatorCallback;

  class Config {
    public:
      enum class Result {
        PERSISTED,
        UNKNOWN_KEY,
        ALREADY_PERSISTED,
        SAME_AS_DEFAULT,
        INVALID_VALUE,
        FAIL_ON_WRITE
      };

      class SetResult {
        public:
          constexpr SetResult(Result result) noexcept : _result(result) {} // NOLINT
          constexpr operator bool() const { return _result == Result::PERSISTED; }
          constexpr operator Result() const { return _result; }

        private:
          Result _result;
      };

      ~Config();

      // Add a new configuration key with its default value
      void configure(const char* key, std::string defaultValue = std::string());

      // starts the config system
      void begin(const char* name = "CONFIG");

      // register a callback to be called when a config value changes
      void listen(ConfigChangeCallback callback) { _changeCallback = callback; }

      // register a callback to be called when the configuration is restored
      void listen(ConfigRestoredCallback callback) { _restoreCallback = callback; }

      // register a global callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(ConfigValidatorCallback callback);

      // register a callback to be called before a config value changes. You can pass a null callback to remove an existing one
      bool setValidator(const char* key, ConfigValidatorCallback callback);

      // returns false if the key is not found
      bool exists(const char* key) const { return std::find(_keys.begin(), _keys.end(), key) != _keys.end(); };

      // get the value of a setting key
      // returns "" if the key is not found, never returns nullptr
      const char* get(const char* key) const { return getString(key).c_str(); }
      const std::string& getString(const char* key) const;
      bool getBool(const char* key) const;
      long getLong(const char* key) const { return std::stol(get(key)); } // NOLINT
      int getInt(const char* key) const { return std::stoi(get(key)); }   // NOLINT
      float getFloat(const char* key) const { return std::stof(get(key)); }
      bool isEmpty(const char* key) const { return get(key)[0] == '\0'; }
      bool isEqual(const char* key, const std::string& value) const { return get(key) == value; }
      bool isEqual(const char* key, const char* value) const { return strcmp(get(key), value) == 0; }

      const SetResult set(const char* key, std::string value, bool fireChangeCallback = true);
      bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true);
      bool setBool(const char* key, bool value) { return set(key, value ? MYCILA_CONFIG_VALUE_TRUE : MYCILA_CONFIG_VALUE_FALSE); }

      bool unset(const char* key, bool fireChangeCallback = true);

      bool isPasswordKey(const char* key) const;
      bool isEnableKey(const char* key) const;

      void backup(Print& out); // NOLINT
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
      mutable std::map<const char*, std::string> _defaults;
      mutable std::map<const char*, std::string> _cache;
      mutable std::map<const char*, ConfigValidatorCallback> _validators;
      const std::string empty;

#ifdef MYCILA_CONFIG_SUPPORT_LONG_KEYS
      mutable std::map<const char*, std::string> _hashedKeys;
      const char* _prefKey(const char* key) const { return strlen(key) > 15 ? _hashedKey(key).c_str() : key; }
      const std::string& _hashedKey(const char* key) const;
#else
      const char* _prefKey(const char* key) const { return key; }
#endif
  };
} // namespace Mycila
