// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <Preferences.h>
#include <map>
#include <vector>

#ifdef MYCILA_JSON_SUPPORT
#include <ArduinoJson.h>
#endif

#define MYCILA_CONFIG_VERSION "3.0.1"
#define MYCILA_CONFIG_VERSION_MAJOR 3
#define MYCILA_CONFIG_VERSION_MINOR 0
#define MYCILA_CONFIG_VERSION_REVISION 1

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
  typedef std::function<void(const String& key, const String& oldValue, const String& newValue)> ConfigChangeCallback;
  typedef std::function<void()> ConfigRestoredCallback;

  class Config {
    public:
      // List of supported configuration keys
      std::vector<const char*> keys;

    public:
      ~Config();

      // Add a new configuration key with its default value
      void configure(const char* key, const String& defaultValue1 = emptyString);

      // starts the config system, eventually with an expected number of settings to reserve the correct amount of memory and avoid reallocations
      void begin(const size_t expectedKeyCount = 0, const char* name = "CONFIG");

      // register a callback to be called when a config value changes
      void listen(ConfigChangeCallback callback) { _changeCallback = callback; }

      // register a callback to be called when the configuration is restored
      void listen(ConfigRestoredCallback callback) { _restoreCallback = callback; }

      String get(const char* key);

      inline bool getBool(const char* key) {
        const String val = get(key);
        return val == "true" || val == "1" || val == "on" || val == "yes";
      }

      bool set(const char* key, const String& value, bool fireChangeCallback = true);
      bool set(const std::map<const char*, String>& settings, bool fireChangeCallback = true);
      inline bool setBool(const char* key, bool value) { return set(key, value ? "true" : "false"); }

      inline bool unset(const char* key, bool fireChangeCallback = true) { return set(key, emptyString, fireChangeCallback); }

      bool isPasswordKey(const char* key) const;
      bool isEnableKey(const char* key) const;

      String backup();
      bool restore(const String& data);
      bool restore(const std::map<const char*, String>& settings);

      // clear all saved settings and current cache
      void clear();

      // this method can be used to find the right pointer to a supported key given a random buffer
      const char* keyRef(const char* buffer) const;

#ifdef MYCILA_JSON_SUPPORT
      void toJson(const JsonObject& root);
#endif

    private:
      Preferences _prefs;
      ConfigChangeCallback _changeCallback = nullptr;
      ConfigRestoredCallback _restoreCallback = nullptr;
      std::map<const char*, String> _defaults;
      std::map<const char*, String> _cache;
  };
} // namespace Mycila
