// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include "MycilaConfig.h"

// #include <esp_memory_utils.h>
#include <assert.h>
#include <soc/soc.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

// Copy of ESP-IDF esp_ptr_in_drom in #include <esp_memory_utils.h>
__attribute__((always_inline)) inline static bool _my_esp_ptr_in_rom(const void* p) {
  intptr_t ip = (intptr_t)p;
  return
#if CONFIG_IDF_TARGET_ARCH_RISCV && SOC_DROM_MASK_LOW != SOC_IROM_MASK_LOW
    (ip >= SOC_DROM_MASK_LOW && ip < SOC_DROM_MASK_HIGH) ||
#endif
    (ip >= SOC_IROM_MASK_LOW && ip < SOC_IROM_MASK_HIGH);
}

// Copy of ESP-IDF esp_ptr_in_rom in #include <esp_memory_utils.h>
__attribute__((always_inline)) inline static bool _my_esp_ptr_in_drom(const void* p) {
  int32_t drom_start_addr = SOC_DROM_LOW;
#if CONFIG_ESP32S3_DATA_CACHE_16KB
  drom_start_addr += 0x4000;
#endif

  return ((intptr_t)p >= drom_start_addr && (intptr_t)p < SOC_DROM_HIGH);
}

__attribute__((always_inline)) inline static bool _isFlashString(const char* str) { return _my_esp_ptr_in_drom(str) || _my_esp_ptr_in_rom(str); }

////////////
// STATIC //
////////////

const Mycila::Config::Value Mycila::Config::VOID{};

/////////
// Str //
/////////

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

Mycila::Config::Str::Str(const Str& other) {
  if (other._buffer) {
    if (_isFlashString(other._buffer)) {
      _buffer = other._buffer;
    } else {
      _buffer = new char[strlen(other._buffer) + 1];
      strcpy(_buffer, other._buffer); // NOLINT
    }
  }
}

Mycila::Config::Str& Mycila::Config::Str::operator=(const Str& other) {
  if (this != &other) {
    if (!_isFlashString(_buffer)) {
      delete[] _buffer;
    }
    _buffer = nullptr;
    if (other._buffer) {
      if (_isFlashString(other._buffer)) {
        _buffer = other._buffer;
      } else {
        _buffer = new char[strlen(other._buffer) + 1];
        strcpy(_buffer, other._buffer); // NOLINT
      }
    }
  }
  return *this;
}

bool Mycila::Config::Str::inFlash() const {
  return _isFlashString(_buffer);
}

size_t Mycila::Config::Str::heapUsage() const {
  return _isFlashString(_buffer) ? 0 : strlen(_buffer) + 1;
}

////////////
// Config //
////////////

bool Mycila::Config::begin(const char* name, bool preload) {
  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Initializing Config System: %s...", name);
  if (!_storage->begin(name)) {
    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Failed to initialize storage backend!");
    return false;
  }
  if (preload) {
    ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Preloading Config System: %s...", name);
    for (auto& key : _keys) {
      auto value = _storage->loadString(key.name);
      if (value.has_value()) {
        _cache[key.name] = std::move(value.value());
        ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "get(%s): CACHED", key.name);
      }
    }
  }
  return true;
}

bool Mycila::Config::setValidator(ValidatorCallback callback) {
  if (callback) {
    _globalValidatorCallback = std::move(callback);
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(callback)");
  } else {
    _globalValidatorCallback = nullptr;
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(nullptr)");
  }
  return true;
}

bool Mycila::Config::setValidator(const char* key, ValidatorCallback callback) {
  Key* k = const_cast<Key*>(this->key(key));

  // check if the key is valid
  if (k == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "setValidator(%s): Unknown key!", key);
    return false;
  }

  if (callback) {
    _validators[key] = std::move(callback);
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, callback)", key);
  } else {
    _validators.erase(key);
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "setValidator(%s, nullptr)", key);
  }

  return true;
}

bool Mycila::Config::configured(const char* key) const {
  return this->key(key) != nullptr;
}

const char* Mycila::Config::keyRef(const char* buffer) const {
  const Key* k = this->key(buffer);
  return k ? k->name : nullptr;
}

const Mycila::Config::Key* Mycila::Config::key(const char* buffer) const {
  auto it = std::lower_bound(_keys.begin(), _keys.end(), buffer, [](const Key& a, const char* b) { return strcmp(a.name, b) < 0; });
  return it != _keys.end() && (it->name == buffer || strcmp(it->name, buffer) == 0) ? &(*it) : nullptr;
}

const Mycila::Config::Result Mycila::Config::_set(const char* key, Value value, bool fireChangeCallback) {
  const Key* k = this->key(key);

  // check if the key is valid
  if (k == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_UNKNOWN_KEY", key);
    return Mycila::Config::Status::ERR_UNKNOWN_KEY;
  }

  // check if the type valid
  if (value.index() != k->defaultValue.index()) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_INVALID_TYPE", key);
    return Mycila::Config::Status::ERR_INVALID_TYPE;
  }

  const bool keyPersisted = stored(key);

  // key not there and set to default value
  if (!keyPersisted && k->defaultValue == value) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): DEFAULTED", key);
    return Mycila::Config::Status::DEFAULTED;
  }

  // check if we have a global validator
  // and check if the value is valid
  if (_globalValidatorCallback && !_globalValidatorCallback(key, value)) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_INVALID_VALUE", key);
    return Mycila::Config::Status::ERR_INVALID_VALUE;
  }

  // check if we have a specific validator for the key
  auto it = _validators.find(k->name);
  if (it != _validators.end()) {
    // check if the value is valid
    if (!it->second(k->name, value)) {
      ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_INVALID_VALUE", key);
      return Mycila::Config::Status::ERR_INVALID_VALUE;
    }
  }

  const bool stored = std::visit(
    [&](auto&& arg) -> bool {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, bool>) {
        return _storage->storeBool(key, arg);
      } else if constexpr (std::is_same_v<T, int8_t>) {
        return _storage->storeI8(key, arg);
      } else if constexpr (std::is_same_v<T, uint8_t>) {
        return _storage->storeU8(key, arg);
      } else if constexpr (std::is_same_v<T, int16_t>) {
        return _storage->storeI16(key, arg);
      } else if constexpr (std::is_same_v<T, uint16_t>) {
        return _storage->storeU16(key, arg);
      } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
        return _storage->storeI32(key, arg);
      } else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
        return _storage->storeU32(key, arg);
#if MYCILA_CONFIG_USE_LONG_LONG
      } else if constexpr (std::is_same_v<T, int64_t>) {
        return _storage->storeI64(key, arg);
      } else if constexpr (std::is_same_v<T, uint64_t>) {
        return _storage->storeU64(key, arg);
#endif
      } else if constexpr (std::is_same_v<T, float>) {
        return _storage->storeFloat(key, arg);
#if MYCILA_CONFIG_USE_DOUBLE
      } else if constexpr (std::is_same_v<T, double>) {
        return _storage->storeDouble(key, arg);
#endif
      } else if constexpr (std::is_same_v<T, Str>) {
        return _storage->storeString(key, arg.c_str());
      }
      return false;
    },
    value);

  // update failed ?
  if (stored) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): PERSISTED", key);
  } else {
    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "set(%s): ERR_FAIL_ON_WRITE", key);
    return Mycila::Config::Status::ERR_FAIL_ON_WRITE;
  }

  _cache[key] = std::move(value);
  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "set(%s): CACHED", key);

  if (fireChangeCallback && _changeCallback) {
    // NOTE: The 'value' pointer passed to the callback is only valid during this callback execution.
    // Do NOT store or use the pointer after the callback returns.
    _changeCallback(key, _cache[key]);
  }

  return Mycila::Config::Status::PERSISTED;
}

bool Mycila::Config::set(std::map<const char*, Value> settings, bool fireChangeCallback) {
  bool updates = false;
  // start restoring settings
  for (const auto& k : _keys)
    if (!isEnableKey(k.name) && settings.find(k.name) != settings.end())
      updates |= _set(k.name, std::move(settings.at(k.name)), fireChangeCallback).isStorageUpdated();
  // then restore settings enabling/disabling a feature
  for (const auto& k : _keys)
    if (isEnableKey(k.name) && settings.find(k.name) != settings.end())
      updates |= _set(k.name, std::move(settings.at(k.name)), fireChangeCallback).isStorageUpdated();
  return updates;
}

const Mycila::Config::Value& Mycila::Config::_get(const char* key) const {
  const Key* k = this->key(key);

  // check if key is configured
  if (k == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "get(%s): ERR_UNKNOWN_KEY", key);
    throw std::runtime_error("Unknown key");
  }

  // check if we have a cached value
  auto it = _cache.find(key);
  if (it != _cache.end()) {
    ESP_LOGV(MYCILA_CONFIG_LOG_TAG, "get(%s): CACHE HIT", key);
    return it->second;
  }

  std::optional<Value> value = std::visit(
    [&](auto&& def) -> std::optional<Value> {
      using T = std::decay_t<decltype(def)>;
      if constexpr (std::is_same_v<T, bool>)
        return _storage->loadBool(key);
      else if constexpr (std::is_same_v<T, int8_t>)
        return _storage->loadI8(key);
      else if constexpr (std::is_same_v<T, uint8_t>)
        return _storage->loadU8(key);
      else if constexpr (std::is_same_v<T, int16_t>)
        return _storage->loadI16(key);
      else if constexpr (std::is_same_v<T, uint16_t>)
        return _storage->loadU16(key);
      else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>)
        return _storage->loadI32(key);
      else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>)
        return _storage->loadU32(key);
#if MYCILA_CONFIG_USE_LONG_LONG
      else if constexpr (std::is_same_v<T, int64_t>)
        return _storage->loadI64(key);
      else if constexpr (std::is_same_v<T, uint64_t>)
        return _storage->loadU64(key);
#endif
      else if constexpr (std::is_same_v<T, float>)
        return _storage->loadFloat(key);
#if MYCILA_CONFIG_USE_DOUBLE
      else if constexpr (std::is_same_v<T, double>)
        return _storage->loadDouble(key);
#endif
      else if constexpr (std::is_same_v<T, Str>)
        return _storage->loadString(key);

      return std::nullopt;
    },
    k->defaultValue);

  // real key exists ?
  if (value.has_value()) {
    // allocate and copy the string to cache
    _cache[key] = std::move(value.value());
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "get(%s): CACHED", key);
    return _cache[key];
  }

  // key does not exist, or not assigned to a value
  ESP_LOGV(MYCILA_CONFIG_LOG_TAG, "get(%s): DEFAULT", key);
  return k->defaultValue;
}

Mycila::Config::Result Mycila::Config::unset(const char* key, bool fireChangeCallback) {
  const Key* k = this->key(key);

  // check if the key is valid
  if (k == nullptr) {
    ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "unset(%s): ERR_UNKNOWN_KEY", key);
    return Mycila::Config::Status::ERR_UNKNOWN_KEY;
  }

  // or not removed
  if (!_storage->remove(key)) {
    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "unset(%s): ERR_FAIL_ON_REMOVE", key);
    return Mycila::Config::Status::ERR_FAIL_ON_REMOVE;
  }

  // key there and to remove
  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "unset(%s): REMOVED", key);
  _cache.erase(key);
  if (fireChangeCallback && _changeCallback)
    _changeCallback(key, VOID);

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
    if (includeDefaults || stored(key.name)) {
      const Value& v = _get(key.name);
      out.print(key.name);
      out.print('=');
      out.print(toString(v).c_str());
      out.print("\n");
    }
  }
}

bool Mycila::Config::restore(const char* data) {
  std::map<const char*, Value> settings;
  for (auto& key : _keys) {
    // int start = data.indexOf(key);
    char* start = strstr(data, key.name);
    if (start) {
      start += strlen(key.name);
      if (*start != '=')
        continue;
      start++;
      char* end = strchr(start, '\r');
      if (!end)
        end = strchr(start, '\n');
      if (!end) {
        ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "restore(%s): Invalid data!", key.name);
        return false;
      }
      std::optional<Value> value = fromString(std::string(start, end - start).c_str(), key.defaultValue);
      if (value.has_value()) {
        settings[key.name] = std::move(value.value());
      } else {
        ESP_LOGW(MYCILA_CONFIG_LOG_TAG, "restore(%s): Invalid data!", key.name);
        return false;
      }
    }
  }
  return restore(std::move(settings));
}

bool Mycila::Config::restore(std::map<const char*, Value> settings) {
  ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "Restoring %d settings...", settings.size());
  bool restored = set(std::move(settings), false);
  if (restored) {
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "Config restored");
    if (_restoreCallback)
      _restoreCallback();
  } else
    ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "No change detected");
  return restored;
}

size_t Mycila::Config::heapUsage() const {
  size_t total = 0;

  // Red-black tree node overhead (without the payload)
  static constexpr size_t rbTreeNodeOverhead = 3 * sizeof(void*) + sizeof(char);

  // variant size
  static constexpr size_t variantSize = sizeof(Value);

  // _keys vector: the vector itself has capacity-based allocation
  // Each Key contains: const char* name (pointer to flash) + Str defaultValue
  if (_keys.capacity() > 0) {
    total += _keys.capacity() * sizeof(Key);

    // Check each Key's defaultValue for heap-allocated strings
    for (const auto& key : _keys) {
      total += variantSize;
      total += std::holds_alternative<Str>(key.defaultValue) ? std::get<Str>(key.defaultValue).heapUsage() : 0;
    }
  }

  // cache map: std::map<const char*, Str>
  // Each node: rb-tree overhead + pair<const char*, Str>
  static constexpr size_t cachePairSize = sizeof(const char*) + sizeof(Str);
  static constexpr size_t cacheNodeSize = rbTreeNodeOverhead + cachePairSize;

  for (const auto& [key, val] : _cache) {
    total += cacheNodeSize; // map node + pair structure
    total += variantSize;
    total += std::holds_alternative<Str>(val) ? std::get<Str>(val).heapUsage() : 0;
  }

  // validators map: std::map<const char*, ValidatorCallback>
  // Each node: rb-tree overhead + pair<const char*, ValidatorCallback>
  static constexpr size_t validatorPairSize = sizeof(const char*) + sizeof(ValidatorCallback);
  total += _validators.size() * (rbTreeNodeOverhead + validatorPairSize);

  return total;
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::Config::toJson(const JsonObject& root) {
  for (auto& key : _keys) {
    const char* value = getString(key.name);
  #ifdef MYCILA_CONFIG_PASSWORD_MASK
    root[key.name] = value[0] == '\0' || !isPasswordKey(key.name) ? value : MYCILA_CONFIG_PASSWORD_MASK;
  #else
    root[key.name] = value;
  #endif // MYCILA_CONFIG_PASSWORD_MASK
  }
}
#endif // MYCILA_JSON_SUPPORT

std::string Mycila::Config::toString(const Value& value) {
  return std::visit(
    [](auto&& arg) -> std::string {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, bool>) {
        return arg ? MYCILA_CONFIG_VALUE_TRUE : MYCILA_CONFIG_VALUE_FALSE;
      } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(arg);
      } else if constexpr (std::is_same_v<T, Config::Str>) {
        return std::string(arg.c_str());
      } else {
        return std::string{};
      }
    },
    value);
}

std::optional<Mycila::Config::Value> Mycila::Config::fromString(const char* str, const Value& defaultValue) {
  return std::visit(
    [&](auto&& variant) -> std::optional<Mycila::Config::Value> {
      using T = std::decay_t<decltype(variant)>;

      if constexpr (std::is_same_v<T, bool>) {
        if (strcmp(str, MYCILA_CONFIG_VALUE_TRUE) == 0) {
          return true;
        }
#if MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING
        if (strcmp(str, "true") == 0 || strcmp(str, "1") == 0 || strcmp(str, "on") == 0 || strcmp(str, "yes") == 0 || strcmp(str, "y") == 0) {
          return true;
        }
#endif
        return false;
      }

      if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
        char* endPtr;
        auto val = std::strtol(str, &endPtr, 10);
        if (*endPtr == '\0') {
          return static_cast<T>(val);
        }
      }

      if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
        char* endPtr;
        auto val = strtoul(str, &endPtr, 10);
        if (*endPtr == '\0') {
          return static_cast<T>(val);
        }
      }

#if MYCILA_CONFIG_USE_LONG_LONG
      if constexpr (std::is_same_v<T, int64_t>) {
        char* endPtr;
        auto val = std::strtoll(str, &endPtr, 10);
        if (*endPtr == '\0') {
          return static_cast<T>(val);
        }
      }

      if constexpr (std::is_same_v<T, uint64_t>) {
        char* endPtr;
        auto val = std::strtoull(str, &endPtr, 10);
        if (*endPtr == '\0') {
          return static_cast<T>(val);
        }
      }
#endif

      if constexpr (std::is_same_v<T, float>) {
        char* endPtr;
        auto val = std::strtof(str, &endPtr);
        if (*endPtr == '\0') {
          return static_cast<T>(val);
        }
      }

#if MYCILA_CONFIG_USE_DOUBLE
      if constexpr (std::is_same_v<T, double>) {
        char* endPtr;
        auto val = std::strtod(str, &endPtr);
        if (*endPtr == '\0') {
          return static_cast<T>(val);
        }
      }
#endif

      if constexpr (std::is_same_v<T, Str>) {
        return Str(str);
      }

      return std::nullopt;
    },
    defaultValue);
}
