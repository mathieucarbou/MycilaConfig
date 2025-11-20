// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <algorithm>

#include "../config/Config.h"

namespace Mycila {
  namespace config {
    class Migration {
      public:
        explicit Migration(Config& config) : _config(&config) {}

        bool begin(const char* name = "CONFIG") { return _config->storage().begin(name); }
        void end() { _config->storage().end(); }

        // returns true if migration succeeded or nothing was done.
        // if true is returned, Config.begin() can be called safely.
        template <typename FROM = Value>
        Result migrate(const char* key, std::function<std::optional<Value>(const FROM& from)> transform) {
          const Key* k = _config->key(key);

          // not key configured, nothing to migrate
          if (k == nullptr) {
            return Status::ERR_UNKNOWN_KEY;
          }

          std::optional<FROM> value = this->_tryload<FROM>(key);

          // not stored, nothing to migrate
          if (!value.has_value()) {
            return Status::ERR_UNKNOWN_KEY;
          }

          // transform value
          std::optional<Value> migrated = transform(value.value());

          // if nothing transformed, remove key
          if (!migrated.has_value()) {
            _config->storage().remove(key);
            ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): REMOVED", key);
            return Status::REMOVED;
          }

          // check type
          if (k->defaultValue.index() != migrated.value().index()) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): ERR_INVALID_TYPE", key);
            return Status::ERR_INVALID_TYPE;
          }

          // store migrated value
          _config->_store(key, migrated.value());

          ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): PERSISTED", key);
          return Status::PERSISTED;
        }

        // returns true if migration succeeded or nothing was done.
        // if true is returned, Config.begin() can be called safely.
        bool migrateFromString() {
          Storage& storage = _config->storage();
          size_t errors = 0;

          for (auto& key : _config->keys()) {
            if (std::holds_alternative<Str>(key.defaultValue)) {
              // skip string keys
              continue;
            }

            // key not persisted ?
            if (!storage.hasKey(key.name)) {
              continue;
            }

            std::optional<Str> value = storage.loadString(key.name);

            // key not a string ?
            if (!value.has_value()) {
              ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "migrate(%s): already migrated", key.name);
              continue;
            }

            // try to convert value to the new type by using fromString() toString()
            const bool success = std::visit(
              [&](auto&& arg) -> bool {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, bool>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to bool", key.name);
                    return false;
                  }
                  if (!storage.storeBool(key.name, converted.value().as<bool>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): bool", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, int8_t>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to int8_t", key.name);
                    return false;
                  }
                  if (!storage.storeI8(key.name, converted.value().as<int8_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): int8_t", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, uint8_t>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to uint8_t", key.name);
                    return false;
                  }
                  if (!storage.storeU8(key.name, converted.value().as<uint8_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): uint8_t", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, int16_t>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to int16_t", key.name);
                    return false;
                  }
                  if (!storage.storeI16(key.name, converted.value().as<int16_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): int16_t", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, uint16_t>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to uint16_t", key.name);
                    return false;
                  }
                  if (!storage.storeU16(key.name, converted.value().as<uint16_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): uint16_t", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to int32_t", key.name);
                    return false;
                  }
                  if (!storage.storeI32(key.name, converted.value().as<int32_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): int32_t", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, unsigned int>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to uint32_t", key.name);
                    return false;
                  }
                  if (!storage.storeU32(key.name, converted.value().as<uint32_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): uint32_t", key.name);
                  return true;

#if MYCILA_CONFIG_USE_LONG_LONG
                } else if constexpr (std::is_same_v<T, int64_t>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to int64_t", key.name);
                    return false;
                  }
                  if (!storage.storeI64(key.name, converted.value().as<int64_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): int64_t", key.name);
                  return true;

                } else if constexpr (std::is_same_v<T, uint64_t>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to uint64_t", key.name);
                    return false;
                  }
                  if (!storage.storeU64(key.name, converted.value().as<uint64_t>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): uint64_t", key.name);
                  return true;
#endif
                } else if constexpr (std::is_same_v<T, float>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to float", key.name);
                    return false;
                  }
                  if (!storage.storeFloat(key.name, converted.value().as<float>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): float", key.name);
                  return true;

#if MYCILA_CONFIG_USE_DOUBLE
                } else if constexpr (std::is_same_v<T, double>) {
                  std::optional<Value> converted = Value::fromString(value.value().c_str(), arg);
                  if (!converted.has_value()) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to convert to double", key.name);
                    return false;
                  }
                  if (!storage.storeDouble(key.name, converted.value().as<double>())) {
                    ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Failed to store", key.name);
                    return false;
                  }
                  ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "migrate(%s): double", key.name);
                  return true;
#endif
                } else if constexpr (std::is_same_v<T, Str>) {
                  // should never go there
                  return true;
                }

                ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): Unsupported type", key.name);
                return false;
              },
              key.defaultValue);

            if (!success) {
              errors++;
            }
          }

          if (errors) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "Migration completed with %d error(s)!", errors);
          } else {
            ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Migration completed successfully.");
          }

          return errors == 0;
        }

      private:
        Config* _config;

        template <typename FROM = Value>
        std::optional<FROM> _tryload(const char* key) const {
          if constexpr (std::is_same_v<FROM, bool>)
            return _config->storage().loadBool(key);
          else if constexpr (std::is_same_v<FROM, int8_t>)
            return _config->storage().loadI8(key);
          else if constexpr (std::is_same_v<FROM, uint8_t>)
            return _config->storage().loadU8(key);
          else if constexpr (std::is_same_v<FROM, int16_t>)
            return _config->storage().loadI16(key);
          else if constexpr (std::is_same_v<FROM, uint16_t>)
            return _config->storage().loadU16(key);
          else if constexpr (std::is_same_v<FROM, int32_t> || std::is_same_v<FROM, int>)
            return _config->storage().loadI32(key);
          else if constexpr (std::is_same_v<FROM, uint32_t> || std::is_same_v<FROM, unsigned int>)
            return _config->storage().loadU32(key);
#if MYCILA_CONFIG_USE_LONG_LONG
          else if constexpr (std::is_same_v<FROM, int64_t>)
            return _config->storage().loadI64(key);
          else if constexpr (std::is_same_v<FROM, uint64_t>)
            return _config->storage().loadU64(key);
#endif
          else if constexpr (std::is_same_v<FROM, float>)
            return _config->storage().loadFloat(key);
#if MYCILA_CONFIG_USE_DOUBLE
          else if constexpr (std::is_same_v<FROM, double>)
            return _config->storage().loadDouble(key);
#endif
          else if constexpr (std::is_same_v<FROM, Str>)
            return _config->storage().loadString(key);
          return std::nullopt;
        }
    };
  } // namespace config
} // namespace Mycila
