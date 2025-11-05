// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "MycilaConfig.h"

#include <assert.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>

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

bool Mycila::Config::setValidator(const char* key, ConfigValidatorCallback callback) {
  // check if the key is valid
  if (!exists(key)) {
    LOGW(TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  if (callback) {
    _validators[key] = callback;
    LOGD(TAG, "setValidator(%s, callback)", key);
  } else {
    _validators.erase(key);
    LOGD(TAG, "setValidator(%s, nullptr)", key);
  }

  return true;
}

void Mycila::Config::configure(const char* key, std::string defaultValue) {
  assert(strlen(key) <= 15);
  _keys.push_back(key);
  std::sort(_keys.begin(), _keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  _defaults[key] = std::move(defaultValue);
  LOGD(TAG, "Config Key '%s' defaults to '%s'", key, _defaults[key].c_str());
}

const std::string& Mycila::Config::getString(const char* key) const {
  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end()) {
    return it->second;
  }

  // not in cache ? is it a real key ?
  if (!exists(key)) {
    LOGW(TAG, "get(%s): Key unknown", key);
    return empty;
  }

  // real key exists ?
  if (_prefs.isKey(key)) {
    const std::string value = _prefs.getString(key).c_str();

    // key exist and is assigned to a value ?
    if (!value.empty()) {
      _cache[key] = value;
      LOGD(TAG, "get(%s): Key cached", key);
      return _cache[key];
    }

    // key exist but is not assigned to a value => remove it
    _prefs.remove(key);
    LOGD(TAG, "get(%s): Key cleaned up", key);
  }

  // key does not exist, or not assigned to a value
  _cache[key] = _defaults[key];
  return _cache[key];
}

bool Mycila::Config::getBool(const char* key) const {
  const std::string& val = get(key);
  return val == "true" || val == "1" || val == "on" || val == "yes";
}

bool Mycila::Config::set(const char* key, std::string value, bool fireChangeCallback) {
  Op op = _set(key, value, fireChangeCallback);
  if (op == Op::SET) {
    _cache[key] = std::move(value);
    LOGD(TAG, "set(%s, %s)", key, value.c_str());
    if (fireChangeCallback && _changeCallback)
      _changeCallback(key, _cache[key]);
    return true;
  }
  return op == Op::UNSET;
}

Mycila::Config::Op Mycila::Config::_set(const char* key, const std::string& value, bool fireChangeCallback) {
  // check if the key is valid
  if (!exists(key)) {
    LOGW(TAG, "set(%s, %s): Unknown key!", key, value.c_str());
    return Op::NOOP;
  }

  const bool keyPersisted = _prefs.isKey(key);

  // key there and set to value
  if (keyPersisted && strcmp(value.c_str(), _prefs.getString(key).c_str()) == 0)
    return Op::NOOP;

  // key not there and set to default value
  if (!keyPersisted && _defaults[key] == value)
    return Op::NOOP;

  // check if we have a validator value
  auto it = _validators.find(key);
  if (it != _validators.end()) {
    // check if the value is valid
    if (!it->second(value)) {
      LOGW(TAG, "set(%s, %s): Invalid value!", key, value.c_str());
      return Op::NOOP;
    }
  }

  // update failed ?
  if (!_prefs.putString(key, value.c_str()))
    return Op::NOOP;

  // to update
  return Op::SET;
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

bool Mycila::Config::unset(const char* key, bool fireChangeCallback) {
  // check if the key is valid
  if (!exists(key)) {
    LOGW(TAG, "unset(%s): Unknown key!", key);
    return false;
  }

  // key not there or not removed
  if (!_prefs.isKey(key) || !_prefs.remove(key))
    return false;

  // key there and to remove
  _cache.erase(key);
  LOGD(TAG, "unset(%s)", key);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, empty);

  return true;
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
  _validators.clear();
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
    const std::string& value = get(key);
  #ifdef MYCILA_CONFIG_PASSWORD_MASK
    root[key] = value.empty() || !isPasswordKey(key) ? value : MYCILA_CONFIG_PASSWORD_MASK;
  #else
    root[key] = value;
  #endif // MYCILA_CONFIG_PASSWORD_MASK
  }
}
#endif // MYCILA_JSON_SUPPORT
