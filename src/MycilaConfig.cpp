// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include "MycilaConfig.h"

#include <assert.h>

#include <algorithm>

#define TAG "CONFIG"

Mycila::ConfigClass::~ConfigClass() {
  _prefs.end();
}

void Mycila::ConfigClass::begin(const size_t expectedKeyCount) {
  ESP_LOGI(TAG, "Initializing Config System...");
  _prefs.begin(TAG, false);
  keys.reserve(expectedKeyCount);
}

void Mycila::ConfigClass::configure(const char* key, const String& defaultValue1) {
  assert(strlen(key) <= 15);
  if (keys.capacity() == keys.size())
    ESP_LOGW(TAG, "Key count is higher than the expectedKeyCount (%u)", keys.capacity());
  keys.push_back(key);
  std::sort(keys.begin(), keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  if (!defaultValue1.isEmpty())
    _defaults[key] = defaultValue1;
}

String Mycila::ConfigClass::get(const char* key) {
  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end())
    return it->second;

  const String value = _prefs.isKey(key) ? _prefs.getString(key, emptyString) : emptyString;

  // value found ? cache and return
  if (!value.isEmpty()) {
    _cache[key] = value;
    return value;
  }

  return _defaults[key];
}

bool Mycila::ConfigClass::set(const char* key, const String& value, bool fire) {
  if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
    ESP_LOGW(TAG, "Set: Unknown key: %s", key);
    return false;
  } else {
    const String oldValue = get(key);
    if (oldValue != value) {
      if (value.isEmpty()) {
        ESP_LOGD(TAG, "Unset %s", key);
        if (_prefs.isKey(key) && _prefs.remove(key)) {
          _cache.erase(key);
          if (fire && _changeCallback)
            _changeCallback(key, oldValue, value);
        }
      } else {
        ESP_LOGD(TAG, "Set %s=%s", key, value.c_str());
        if (_prefs.putString(key, value.c_str())) {
          _cache[key] = value;
          if (fire && _changeCallback)
            _changeCallback(key, oldValue, value);
        }
      }
      return true;
    } else
      return false;
  }
}

bool Mycila::ConfigClass::set(const std::map<const char*, String>& settings, bool fire) {
  bool updates = false;
  // start restoring settings
  for (auto& key : keys)
    if (!isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key), fire);
  // then restore settings enabling/disabling a feature
  for (auto& key : keys)
    if (isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key), fire);
  return updates;
}

String Mycila::ConfigClass::backup() {
  String body;
  body.reserve(2048);
  for (auto& key : keys) {
    body += key;
    body += "=";
    body += get(key);
    body += "\n";
  }
  return body;
}

bool Mycila::ConfigClass::restore(const String& data) {
  std::map<const char*, String> settings;
  for (auto& key : keys) {
    int start = data.indexOf(key);
    if (start >= 0) {
      start += strlen(key);
      if (data[start] != '=')
        continue;
      start++;
      int end = data.indexOf("\r", start);
      if (end < 0)
        end = data.indexOf("\n", start);
      if (end < 0)
        end = data.length();
      String v = data.substring(start, end);
      v.trim();
      settings[key] = v;
    }
  }
  return restore(settings);
}

bool Mycila::ConfigClass::restore(const std::map<const char*, String>& settings) {
  ESP_LOGD(TAG, "Restoring %d settings...", settings.size());
  bool restored = set(settings, false);
  if (restored) {
    ESP_LOGD(TAG, "Config restored");
    if (_restoreCallback)
      _restoreCallback();
  } else
    ESP_LOGD(TAG, "No change detected");
  return restored;
}

void Mycila::ConfigClass::clear() {
  _prefs.clear();
  _cache.clear();
}

bool Mycila::ConfigClass::isPasswordKey(const char* key) const {
  uint32_t len = strlen(key);
  if (len < 4)
    return false;
  return strcmp(key + len - 4, MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) == 0;
}

bool Mycila::ConfigClass::isEnableKey(const char* key) const {
  uint32_t len = strlen(key);
  if (len < 7)
    return false;
  return strcmp(key + len - 7, MYCILA_CONFIG_KEY_ENABLE_SUFFIX) == 0;
}

const char* Mycila::ConfigClass::keyRef(const char* buffer) const {
  for (auto& k : keys)
    if (strcmp(k, buffer) == 0)
      return k;
  return nullptr;
}

#ifdef MYCILA_CONFIG_JSON_SUPPORT
void Mycila::ConfigClass::toJson(const JsonObject& root) const {
  for (auto& key : keys) {
    String value = Mycila::Config.get(key);
#ifdef MYCILA_CONFIG_PASSWORD_MASK
    root[key] = value.isEmpty() || !isPasswordKey(key) ? value : MYCILA_CONFIG_PASSWORD_MASK;
#else
    root[key] = value;
#endif // MYCILA_CONFIG_PASSWORD_MASK
  }
}
#endif // MYCILA_CONFIG_JSON_SUPPORT

namespace Mycila {
  ConfigClass Config;
} // namespace Mycila
