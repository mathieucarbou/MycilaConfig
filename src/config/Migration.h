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

        bool begin(const char* name = "CONFIG") {
          ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Migrating Config: '%s'", name);
          return _config->storage().begin(name);
        }

        void end() {
          _config->storage().end();
          ESP_LOGI(MYCILA_CONFIG_LOG_TAG, "Migration ended");
        }

        // returns true if migration succeeded or nothing was done.
        // if true is returned, Config.begin() can be called safely.
        template <typename FROM = Value>
        Result migrate(const char* key, std::function<std::optional<Value>(const FROM& from)> transform) {
          const Key* k = _config->key(key);

          // not key configured, nothing to migrate
          if (k == nullptr) {
            return Status::ERR_UNKNOWN_KEY;
          }

          std::optional<FROM> value = _config->_load<FROM>(key);

          // not stored or already migrated
          if (!value.has_value()) {
            return Status::ERR_UNKNOWN_KEY;
          }

          // transform value
          std::optional<Value> migrated = transform(value.value());

          // if nothing transformed, remove key
          if (!migrated.has_value()) {
            _config->storage().remove(key);
            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "migrate(%s): REMOVED", key);
            return Status::REMOVED;
          }

          // check type
          if (k->defaultValue.index() != migrated.value().index()) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrate(%s): ERR_INVALID_TYPE", key);
            return Status::ERR_INVALID_TYPE;
          }

          // store migrated value
          _config->_store(key, migrated.value());

          ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "migrate(%s): PERSISTED", key);
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

            // key not a string (already migrated)
            if (!value.has_value()) {
              continue;
            }

            std::optional<Value> converted = Value::fromString(value.value().c_str(), key.defaultValue);

            if (!converted.has_value()) {
              ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrateFromString(%s): Failed to convert from string", key.name);
              errors++;
              continue;
            }

            if (!_config->storage().remove(key.name)) {
              ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrateFromString(%s): Failed to remove old string value", key.name);
              errors++;
              continue;
            }

            if (!_config->_store(key.name, converted.value())) {
              ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrateFromString(%s): Failed to store converted value", key.name);
              errors++;
              continue;
            }

            ESP_LOGD(MYCILA_CONFIG_LOG_TAG, "migrateFromString(%s): PERSISTED", key.name);
          }

          if (errors) {
            ESP_LOGE(MYCILA_CONFIG_LOG_TAG, "migrateFromString(): %d error(s)!", errors);
          }

          return errors == 0;
        }

      private:
        Config* _config;
    };
  } // namespace config
} // namespace Mycila
