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

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#define MYCILA_CONFIG_VERSION          "7.1.0"
#define MYCILA_CONFIG_VERSION_MAJOR    7
#define MYCILA_CONFIG_VERSION_MINOR    1
#define MYCILA_CONFIG_VERSION_REVISION 0

// suffix to use for a setting key enabling a feature
#define MYCILA_CONFIG_KEY_ENABLE_SUFFIX "_enable"

// suffix to use for a setting key representing a password
#define MYCILA_CONFIG_KEY_PASSWORD_SUFFIX "_pwd"

#ifndef MYCILA_CONFIG_SHOW_PASSWORD
  #ifndef MYCILA_CONFIG_PASSWORD_MASK
    #define MYCILA_CONFIG_PASSWORD_MASK "********"
  #endif
#endif

namespace Mycila {
  typedef std::function<void(const char* key, const std::string& newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;

  class Config {
    public:
      ~Config();

      // Add a new configuration key with its default value
      void configure(const char* key, std::string defaultValue = std::string());

      // starts the config system
      void begin(const char* name = "CONFIG");

      // register a callback to be called when a config value changes
      void listen(ConfigChangeCallback callback) { _changeCallback = callback; }

      // register a callback to be called when the configuration is restored
      void listen(ConfigRestoredCallback callback) { _restoreCallback = callback; }

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

      bool set(const char* key, const std::string value, bool fireChangeCallback = true);
      bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true);
      bool setBool(const char* key, bool value) { return set(key, value ? "true" : "false"); }

      bool unset(const char* key, bool fireChangeCallback = true) { return set(key, "", fireChangeCallback); }

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
      enum class Op { NOOP,
                      GET,
                      SET,
                      UNSET };
      ConfigChangeCallback _changeCallback = nullptr;
      ConfigRestoredCallback _restoreCallback = nullptr;
      std::vector<const char*> _keys;
      mutable Preferences _prefs;
      mutable std::map<const char*, std::string> _defaults;
      mutable std::map<const char*, std::string> _cache;
      const std::string empty;

      Op _set(const char* key, const char* value, bool fireChangeCallback);
  };
} // namespace Mycila
