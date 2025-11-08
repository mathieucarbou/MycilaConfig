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

#define TAG "CONFIG"

Mycila::Config::~Config() {
  _prefs.end();
}

void Mycila::Config::begin(const char* name) {
  ESP_LOGI(TAG, "Initializing Config System: %s...", name);
  _prefs.begin(name, false);
}

bool Mycila::Config::setValidator(ConfigValidatorCallback callback) {
  if (callback) {
    _globalValidatorCallback = callback;
    ESP_LOGD(TAG, "setValidator(callback)");
  } else {
    _globalValidatorCallback = nullptr;
    ESP_LOGD(TAG, "setValidator(nullptr)");
  }
  return true;
}

bool Mycila::Config::setValidator(const char* key, ConfigValidatorCallback callback) {
  // check if the key is valid
  if (!exists(key)) {
    ESP_LOGW(TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  if (callback) {
    _validators[key] = callback;
    ESP_LOGD(TAG, "setValidator(%s, callback)", key);
  } else {
    _validators.erase(key);
    ESP_LOGD(TAG, "setValidator(%s, nullptr)", key);
  }

  return true;
}

void Mycila::Config::configure(const char* key, std::string defaultValue) {
#ifndef MYCILA_CONFIG_SUPPORT_LONG_KEYS
  assert(strlen(key) <= 15);
#endif
  _keys.push_back(key);
  std::sort(_keys.begin(), _keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  _defaults[key] = std::move(defaultValue);
  ESP_LOGD(TAG, "Config Key '%s' defaults to '%s'", key, _defaults[key].c_str());
}

const std::string& Mycila::Config::getString(const char* key) const {
  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end()) {
    return it->second;
  }

  // not in cache ? is it a real key ?
  if (!exists(key)) {
    ESP_LOGW(TAG, "get(%s): Key unknown", key);
    return empty;
  }

  // real key exists ?
  if (_prefs.isKey(_prefKey(key))) {
    const std::string value = _prefs.getString(_prefKey(key)).c_str();

    // key exist and is assigned to a value ?
    if (!value.empty()) {
      _cache[key] = value;
      ESP_LOGD(TAG, "get(%s): Key cached", key);
      return _cache[key];
    }

    // key exist but is not assigned to a value => remove it
    _prefs.remove(_prefKey(key));
    ESP_LOGD(TAG, "get(%s): Key cleaned up", key);
  }

  // key does not exist, or not assigned to a value
  _cache[key] = _defaults[key];
  return _cache[key];
}

bool Mycila::Config::getBool(const char* key) const {
  const std::string& val = get(key);
  if (val == MYCILA_CONFIG_VALUE_TRUE) {
    return true;
  }

#if MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING
  if (val == "true" || val == "1" || val == "on" || val == "yes") {
    return true;
  }
#endif

  return false;
}

const Mycila::Config::SetResult Mycila::Config::set(const char* key, std::string value, bool fireChangeCallback) {
  // check if the key is valid
  if (!exists(key)) {
    ESP_LOGW(TAG, "set(%s, %s): UNKNOWN_KEY", key, value.c_str());
    return Mycila::Config::Result::UNKNOWN_KEY;
  }

  const bool keyPersisted = _prefs.isKey(_prefKey(key));

  // key there and set to value
  if (keyPersisted && strcmp(value.c_str(), _prefs.getString(_prefKey(key)).c_str()) == 0) {
    ESP_LOGD(TAG, "set(%s, %s): ALREADY_PERSISTED", key, value.c_str());
    return Mycila::Config::Result::ALREADY_PERSISTED;
  }

  // key not there and set to default value
  if (!keyPersisted && _defaults[key] == value) {
    ESP_LOGD(TAG, "set(%s, %s): SAME_AS_DEFAULT", key, value.c_str());
    return Mycila::Config::Result::SAME_AS_DEFAULT;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(key, value)) {
    ESP_LOGD(TAG, "set(%s, %s): INVALID_VALUE", key, value.c_str());
    return Mycila::Config::Result::INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  auto it = _validators.find(key);
  if (it != _validators.end()) {
    // check if the value is valid
    if (!it->second(key, value)) {
      ESP_LOGD(TAG, "set(%s, %s): INVALID_VALUE", key, value.c_str());
      return Mycila::Config::Result::INVALID_VALUE;
    }
  }

  // update failed ?
  if (!_prefs.putString(_prefKey(key), value.c_str()))
    return Mycila::Config::Result::FAIL_ON_WRITE;

  _cache[key] = std::move(value);
  ESP_LOGD(TAG, "set(%s, %s)", key, _cache[key].c_str());
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, _cache[key]);

  return Mycila::Config::Result::PERSISTED;
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
    ESP_LOGW(TAG, "unset(%s): Unknown key!", key);
    return false;
  }

  // key not there or not removed
  if (!_prefs.isKey(_prefKey(key)) || !_prefs.remove(_prefKey(key)))
    return false;

  // key there and to remove
  _cache.erase(key);
  ESP_LOGD(TAG, "unset(%s)", key);
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
        ESP_LOGW(TAG, "restore(%s): Invalid data!", key);
        return false;
      }
      settings[key] = std::string(start, end - start);
    }
  }
  return restore(settings);
}

bool Mycila::Config::restore(const std::map<const char*, std::string>& settings) {
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

void Mycila::Config::clear() {
  _prefs.clear();
  _cache.clear();

#ifdef MYCILA_CONFIG_SUPPORT_LONG_KEYS
  _hashedKeys.clear();
#endif
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

#ifdef MYCILA_CONFIG_SUPPORT_LONG_KEYS
const std::string& Mycila::Config::_hashedKey(const char* key) const {
  // check if we have a cached value
  auto it = _hashedKeys.find(key);
  if (it != _hashedKeys.end()) {
    return it->second;
  }

  uint8_t hash[16];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const uint8_t*)key, strlen(key));
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  std::string hashedKey;
  hashedKey.reserve(14);
  static const char hexDigits[] = "0123456789abcdef";
  for (size_t i = 0; i < 7; i++) {
    hashedKey += hexDigits[hash[i] >> 4];
    hashedKey += hexDigits[hash[i] & 0xF];
  }
  _hashedKeys[key] = std::move(hashedKey);

  return _hashedKeys[key];
}
#endif
