// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include "MycilaConfig.h"

#include <assert.h>

#include <algorithm>
#include <string>
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

void Mycila::Config::begin(const char* name) {
  LOGI(TAG, "Initializing Config System: %s...", name);
  _prefs.begin(name, false);
}

void Mycila::Config::configure(const char* key, const char* defaultValue) {
  assert(strlen(key) <= 15);
  _keys.push_back(key);
  std::sort(_keys.begin(), _keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  if (!defaultValue)
    defaultValue = "";
  _defaults[key] = defaultValue;
  LOGD(TAG, "Config Key '%s' defaults to '%s'", key, defaultValue);
}

const char* Mycila::Config::get(const char* key) const {
  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end()) {
    return it->second.c_str();
  }

  // not in cache ? is it a real key ?
  if (std::find(_keys.begin(), _keys.end(), key) == _keys.end()) {
    LOGW(TAG, "get(%s): Key unknown", key);
    return "";
  }

  // real key exists ?
  if (_prefs.isKey(key)) {
    const String value = _prefs.getString(key).c_str();

    // key exist and is assigned to a value ?
    if (!value.isEmpty()) {
      _cache[key] = value.c_str();
      LOGD(TAG, "get(%s): Key cached", key);
      return _cache[key].c_str();
    }

    // key exist but is not assigned to a value => remove it
    _prefs.remove(key);
    LOGD(TAG, "get(%s): Key cleaned up", key);
  }

  // key does not exist, or not assigned to a value
  _cache[key] = _defaults[key];
  return _cache[key].c_str();
}

bool Mycila::Config::getBool(const char* key) const {
  const char* val = get(key);
  return strcasecmp(val, "true") == 0 ||
         strcmp(val, "1") == 0 ||
         strcmp(val, "on") == 0 ||
         strcmp(val, "yes") == 0;
}

bool Mycila::Config::set(const char* key, const char* value, bool fireChangeCallback) {
  const bool del = value == nullptr || strlen(value) == 0;

  // check if the key is valid
  if (std::find(_keys.begin(), _keys.end(), key) == _keys.end()) {
    if (del) {
      LOGW(TAG, "unset(%s): Unknown key!", key);
    } else {
      LOGW(TAG, "set(%s, %s): Unknown key!", key, value);
    }
    return false;
  }

  // requested deletion ?
  if (del) {
    // key not there or not removed
    if (!_prefs.isKey(key) || !_prefs.remove(key))
      return false;

    // key there and removed
    _cache.erase(key);
    LOGD(TAG, "unset(%s)", key);
    if (fireChangeCallback && _changeCallback)
      _changeCallback(key, "");
    return true;
  }

  const bool keyPersisted = _prefs.isKey(key);

  // key there and set to value
  if (keyPersisted && strcmp(value, _prefs.getString(key).c_str()) == 0)
    return false;

  // key not there and set to default value
  if (!keyPersisted && _defaults[key] == value)
    return false;

  // update failed ?
  if (!_prefs.putString(key, value))
    return false;

  // updated
  _cache[key] = value;
  LOGD(TAG, "set(%s, %s)", key, value);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, _cache[key].c_str());
  return true;
}

bool Mycila::Config::set(const std::map<const char*, std::string>& settings, bool fireChangeCallback) {
  bool updates = false;
  // start restoring settings
  for (auto& key : _keys)
    if (!isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key).c_str(), fireChangeCallback);
  // then restore settings enabling/disabling a feature
  for (auto& key : _keys)
    if (isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key).c_str(), fireChangeCallback);
  return updates;
}

void Mycila::Config::backup(Print& out) {
  for (auto& key : _keys) {
    out.print(key);
    out.print('=');
    out.print(get(key));
    out.print("\n");
  }
}

bool Mycila::Config::restore(const char* data) {
  std::map<const char*, std::string> settings;
  for (auto& key : _keys) {
    // int start = data.indexOf(key);
    char* start = strstr(data, key);
    if (start) {
      start += strlen(key);
      if (*start != '=')
        continue;
      start++;
      char* end = strchr(start, '\r');
      if (!end)
        end = strchr(start, '\n');
      if (!end) {
        LOGW(TAG, "restore(%s): Invalid data!", key);
        return false;
      }
      settings[key] = std::string(start, end - start);
    }
  }
  return restore(settings);
}

bool Mycila::Config::restore(const std::map<const char*, std::string>& settings) {
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
  for (auto& k : _keys)
    if (strcmp(k, buffer) == 0)
      return k;
  return nullptr;
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::Config::toJson(const JsonObject& root) {
  for (auto& key : _keys) {
    const char* value = get(key);
  #ifdef MYCILA_CONFIG_PASSWORD_MASK
    root[key] = strlen(value) == 0 || !isPasswordKey(key) ? value : MYCILA_CONFIG_PASSWORD_MASK;
  #else
    root[key] = value;
  #endif // MYCILA_CONFIG_PASSWORD_MASK
  }
}
#endif // MYCILA_JSON_SUPPORT
