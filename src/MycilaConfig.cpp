// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "MycilaConfig.h"

#include <assert.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>

#define TAG "CONFIG"

// Copy of ESP-IDF esp_ptr_in_drom
__attribute__((always_inline)) inline static bool _my_esp_ptr_in_rom(const void* p) {
  intptr_t ip = (intptr_t)p;
  return
#if CONFIG_IDF_TARGET_ARCH_RISCV && SOC_DROM_MASK_LOW != SOC_IROM_MASK_LOW
    (ip >= SOC_DROM_MASK_LOW && ip < SOC_DROM_MASK_HIGH) ||
#endif
    (ip >= SOC_IROM_MASK_LOW && ip < SOC_IROM_MASK_HIGH);
}

// Copy of ESP-IDF esp_ptr_in_rom
__attribute__((always_inline)) inline static bool _my_esp_ptr_in_drom(const void* p) {
  int32_t drom_start_addr = SOC_DROM_LOW;
#if CONFIG_ESP32S3_DATA_CACHE_16KB
  drom_start_addr += 0x4000;
#endif

  return ((intptr_t)p >= drom_start_addr && (intptr_t)p < SOC_DROM_HIGH);
}

__attribute__((always_inline)) inline static bool _isFlashString(const char* str) { return _my_esp_ptr_in_drom(str) || _my_esp_ptr_in_rom(str); }

/////////
// Str //
/////////

Mycila::Config::Str::Str(size_t length) {
  _buffer = new char[length + 1];
  _buffer[0] = '\0';
}

Mycila::Config::Str::Str(const char* str) {
  if (_isFlashString(str)) {
    _buffer = const_cast<char*>(str);
  } else {
    _buffer = new char[strlen(str) + 1];
    strcpy(_buffer, str); // NOLINT
  }
}

Mycila::Config::Str::~Str() {
  if (!_isFlashString(_buffer)) {
    delete[] _buffer;
  }
}

Mycila::Config::Str::Str(Str&& other) noexcept : _buffer(other._buffer) {
  other._buffer = nullptr;
}

Mycila::Config::Str& Mycila::Config::Str::operator=(Str&& other) noexcept {
  if (this != &other) {
    // Clean up existing resource
    if (_buffer && !_isFlashString(_buffer)) {
      delete[] _buffer;
    }
    // Move data from other
    _buffer = other._buffer;
    // Nullify other
    other._buffer = nullptr;
  }
  return *this;
}

inline bool Mycila::Config::Str::inFlash() const {
  return _isFlashString(_buffer);
}

size_t Mycila::Config::Str::heapUsage() const {
  return _isFlashString(_buffer) ? 4 : 4 + strlen(_buffer) + 1;
}

////////////
// Config //
////////////

bool Mycila::Config::begin(const char* name) {
  ESP_LOGI(TAG, "Initializing Config System: %s...", name);
  return _storage->begin(name);
}

bool Mycila::Config::configure(const char* key, const char* defaultValue, ConfigValidatorCallback callback) {
  if (strlen(key) > 15) {
    return false;
  }

  _keys.push_back(key);
  std::sort(_keys.begin(), _keys.end(), [](const char* a, const char* b) { return strcmp(a, b) < 0; });

  _defaults.insert_or_assign(key, Str(defaultValue));

  if (_defaults.at(key).inFlash()) {
    ESP_LOGD(TAG, "Config Key '%s' defaults to string pointer: '%s'", key, defaultValue);

  } else {
    ESP_LOGD(TAG, "Config Key '%s' defaults to string buffer: '%s'", key, defaultValue);
  }

  if (callback) {
    _validators[key] = std::move(callback);
    ESP_LOGD(TAG, "setValidator(%s, callback)", key);
  }

  return true;
}

bool Mycila::Config::setValidator(ConfigValidatorCallback callback) {
  if (callback) {
    _globalValidatorCallback = std::move(callback);
    ESP_LOGD(TAG, "setValidator(callback)");
  } else {
    _globalValidatorCallback = nullptr;
    ESP_LOGD(TAG, "setValidator(nullptr)");
  }
  return true;
}

bool Mycila::Config::setValidator(const char* key, ConfigValidatorCallback callback) {
  // check if the key is valid
  if (!configured(key)) {
    ESP_LOGW(TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  if (callback) {
    _validators[key] = std::move(callback);
    ESP_LOGD(TAG, "setValidator(%s, callback)", key);
  } else {
    _validators.erase(key);
    ESP_LOGD(TAG, "setValidator(%s, nullptr)", key);
  }

  return true;
}

bool Mycila::Config::configured(const char* key) const {
  // just compare pointer values for speed
  return std::find(_keys.begin(), _keys.end(), key) != _keys.end();
};

const char* Mycila::Config::keyRef(const char* buffer) const {
  auto it = std::lower_bound(_keys.begin(), _keys.end(), buffer, [](const char* a, const char* b) { return strcmp(a, b) < 0; });
  return it != _keys.end() && strcmp(*it, buffer) == 0 ? static_cast<const char*>(*it) : nullptr;
}

const Mycila::Config::Result Mycila::Config::set(const char* key, const char* value, bool fireChangeCallback) {
  if (value == nullptr) {
    return unset(key, fireChangeCallback);
  }

  // check if the key is valid
  if (!configured(key)) {
    ESP_LOGW(TAG, "set(%s, %s): ERR_UNKNOWN_KEY", key, value);
    return Mycila::Config::Status::ERR_UNKNOWN_KEY;
  }

  const bool keyPersisted = stored(key);

  // key not there and set to default value
  if (!keyPersisted && strcmp(value, _defaults.at(key).c_str()) == 0) {
    ESP_LOGD(TAG, "set(%s, %s): DEFAULTED", key, value);
    return Mycila::Config::Status::DEFAULTED;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(key, value)) {
    ESP_LOGD(TAG, "set(%s, %s): ERR_INVALID_VALUE", key, value);
    return Mycila::Config::Status::ERR_INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  auto it = _validators.find(key);
  if (it != _validators.end()) {
    // check if the value is valid
    if (!it->second(key, value)) {
      ESP_LOGD(TAG, "set(%s, %s): ERR_INVALID_VALUE", key, value);
      return Mycila::Config::Status::ERR_INVALID_VALUE;
    }
  }

  // update failed ?
  if (_storage->storeString(key, value)) {
    ESP_LOGD(TAG, "set(%s, %s): PERSISTED", key, value);
  } else {
    ESP_LOGE(TAG, "set(%s, %s): ERR_FAIL_ON_WRITE", key, value);
    return Mycila::Config::Status::ERR_FAIL_ON_WRITE;
  }

  _cache.insert_or_assign(key, Str(value));
  ESP_LOGD(TAG, "set(%s, %s): CACHED", key, value);

  if (fireChangeCallback && _changeCallback)
    // NOTE: The 'value' pointer passed to the callback is only valid during this callback execution.
    // Do NOT store or use the pointer after the callback returns.
    _changeCallback(key, value);
  return Mycila::Config::Status::PERSISTED;
}

bool Mycila::Config::set(const std::map<const char*, std::string>& settings, bool fireChangeCallback) {
  bool updates = false;
  // start restoring settings
  for (auto& key : _keys)
    if (!isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key).c_str(), fireChangeCallback).isStorageUpdated();
  // then restore settings enabling/disabling a feature
  for (auto& key : _keys)
    if (isEnableKey(key) && settings.find(key) != settings.end())
      updates |= set(key, settings.at(key).c_str(), fireChangeCallback).isStorageUpdated();
  return updates;
}

bool Mycila::Config::getBool(const char* key) const {
  std::string val = get(key);
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

const char* Mycila::Config::get(const char* key) const {
  // check if key is configured
  if (!configured(key)) {
    ESP_LOGW(TAG, "get(%s): ERR_UNKNOWN_KEY", key);
    return nullptr;
  }

  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end()) {
    ESP_LOGV(TAG, "get(%s): CACHE HIT", key);
    return it->second.c_str();
  }

  std::optional<Str> value = _storage->load(key);

  // real key exists ?
  if (value.has_value()) {
    // allocate and copy the string to cache
    _cache.insert_or_assign(key, std::move(value.value()));
    ESP_LOGD(TAG, "get(%s): CACHED", key);
    return _cache.at(key).c_str();
  }

  // key does not exist, or not assigned to a value
  ESP_LOGV(TAG, "get(%s): DEFAULT", key);
  return _defaults.at(key).c_str();
}

Mycila::Config::Result Mycila::Config::unset(const char* key, bool fireChangeCallback) {
  // check if the key is valid
  if (!configured(key)) {
    ESP_LOGW(TAG, "unset(%s): ERR_UNKNOWN_KEY", key);
    return Mycila::Config::Status::ERR_UNKNOWN_KEY;
  }

  // or not removed
  if (!_storage->remove(key)) {
    ESP_LOGE(TAG, "unset(%s): ERR_FAIL_ON_REMOVE", key);
    return Mycila::Config::Status::ERR_FAIL_ON_REMOVE;
  }

  // key there and to remove
  ESP_LOGD(TAG, "unset(%s): REMOVED", key);
  _cache.erase(key);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, "");

  return Mycila::Config::Status::REMOVED;
}

void Mycila::Config::clear() {
  _storage->removeAll();
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

void Mycila::Config::backup(Print& out, bool includeDefaults) {
  for (auto& key : _keys) {
    if (includeDefaults || stored(key)) {
      const char* v = get(key);
      out.print(key);
      out.print('=');
      out.print(v);
      out.print("\n");
    }
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

size_t Mycila::Config::heapUsage() const {
  size_t total = 0;

  // std::map is implemented as a red-black tree with nodes allocated on heap
  // Each node contains: 3 pointers (parent, left, right), 1 byte (color), and the key-value pair
  // The key-value pair is std::pair<const char* const, Str>
  // - const char* const: pointer (not counted, points to flash)
  // - Str: contains char* pointer + bool flag

  // Red-black tree node overhead (without the payload)
  static constexpr size_t rbTreeNodeOverhead = 3 * sizeof(void*) + sizeof(char);

  // Size of the pair stored in each node (key pointer + Str object)
  static constexpr size_t pairSize = sizeof(const char*) + sizeof(Str);

  // Total per map node
  static constexpr size_t mapNodeSize = rbTreeNodeOverhead + pairSize;

  // defaults map
  for (const auto& [key, val] : _defaults) {
    total += mapNodeSize; // map node + pair structure
    total += val.heapUsage();
  }

  // cache map
  for (const auto& [key, val] : _cache) {
    total += mapNodeSize; // map node + pair structure
    total += val.heapUsage();
  }

  // validators map (if any)
  // Each node: rb-tree overhead + pair<const char*, ConfigValidatorCallback>
  static constexpr size_t validatorPairSize = sizeof(const char*) + sizeof(ConfigValidatorCallback);
  total += _validators.size() * (rbTreeNodeOverhead + validatorPairSize);

  // _keys vector: the vector itself has capacity-based allocation
  // Only counts if capacity > 0 (heap-allocated storage)
  if (_keys.capacity() > 0) {
    total += _keys.capacity() * sizeof(const char*);
  }

  return total;
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
