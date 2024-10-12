// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include "MycilaConfig.h"

#include <assert.h>

#include <algorithm>
#include <map>

#ifdef MYCILA_LOGGER_SUPPORT
  #include <MycilaLogger.h>
extern Mycila::Logger logger;
  #define LOGD(tag, format, ...) logger.debug(tag, format, ##__VA_ARGS__)
  #define LOGI(tag, format, ...) logger.info(tag, format, ##__VA_ARGS__)
  #define LOGW(tag, format, ...) logger.warn(tag, format, ##__VA_ARGS__)
  #define LOGE(tag, format, ...) logger.error(tag, format, ##__VA_ARGS__)
#else
  #define LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
  #define LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
  #define LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
  #define LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#endif

#define TAG "CONFIG"

Mycila::Config::~Config() {
  _prefs.end();
}

void Mycila::Config::begin(const size_t expectedKeyCount, const char* name) {
  LOGI(TAG, "Initializing Config System: %s...", name);
  _prefs.begin(name, false);
  keys.reserve(expectedKeyCount);
}

void Mycila::Config::configure(const char* key, const String& defaultValue1) {
  assert(strlen(key) <= 15);
  if (keys.capacity() == keys.size())
    LOGW(TAG, "Key count is higher than the expectedKeyCount (%d)", keys.capacity());
  keys.push_back(key);
  std::sort(keys.begin(), keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  if (!defaultValue1.isEmpty())
    _defaults[key] = defaultValue1;
}

String Mycila::Config::get(const char* key) {
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

bool Mycila::Config::set(const char* key, const String& value, bool fire) {
  if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
    LOGW(TAG, "Set: Unknown key: %s", key);
    return false;
  } else {
    const String oldValue = get(key);
    if (oldValue != value) {
      if (value.isEmpty()) {
        LOGD(TAG, "Unset %s", key);
        if (_prefs.isKey(key) && _prefs.remove(key)) {
          _cache.erase(key);
          if (fire && _changeCallback)
            _changeCallback(key, oldValue, value);
        }
      } else {
        LOGD(TAG, "Set %s=%s", key, value.c_str());
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

bool Mycila::Config::set(const std::map<const char*, String>& settings, bool fire) {
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

void Mycila::Config::backup(String& content) {
  for (auto& key : keys) {
    content.concat(key);
    content.concat("=");
    content.concat(get(key));
    content.concat("\n");
  }
}

bool Mycila::Config::restore(const String& data) {
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

bool Mycila::Config::restore(const std::map<const char*, String>& settings) {
  LOGD(TAG, "Restoring %d settings...", settings.size());
  bool restored = set(settings, false);
  if (restored) {
    LOGD(TAG, "Config restored");
    if (_restoreCallback)
      _restoreCallback();
  } else
    LOGD(TAG, "No change detected");
  return restored;
}

void Mycila::Config::clear() {
  _prefs.clear();
  _cache.clear();
}

bool Mycila::Config::isPasswordKey(const char* key) const {
  uint32_t len = strlen(key);
  if (len < 4)
    return false;
  return strcmp(key + len - 4, MYCILA_CONFIG_KEY_PASSWORD_SUFFIX) == 0;
}

bool Mycila::Config::isEnableKey(const char* key) const {
  uint32_t len = strlen(key);
  if (len < 7)
    return false;
  return strcmp(key + len - 7, MYCILA_CONFIG_KEY_ENABLE_SUFFIX) == 0;
}

const char* Mycila::Config::keyRef(const char* buffer) const {
  for (auto& k : keys)
    if (strcmp(k, buffer) == 0)
      return k;
  return nullptr;
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::Config::toJson(const JsonObject& root) {
  for (auto& key : keys) {
    String value = get(key);
  #ifdef MYCILA_CONFIG_PASSWORD_MASK
    root[key] = value.isEmpty() || !isPasswordKey(key) ? value : MYCILA_CONFIG_PASSWORD_MASK;
  #else
    root[key] = value;
  #endif // MYCILA_CONFIG_PASSWORD_MASK
  }
}
#endif // MYCILA_JSON_SUPPORT
