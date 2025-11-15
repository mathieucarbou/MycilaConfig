// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <MycilaConfig.h>
#include <Preferences.h>

#include <type_traits>
#include <variant>

namespace Mycila {
  class PreferencesStorage : public Config::Storage {
    public:
      PreferencesStorage() = default;
      virtual ~PreferencesStorage() { _prefs.end(); };
      bool begin(const char* name) override { return _prefs.begin(name, false); }
      bool hasKey(const char* key) const override { return _prefs.isKey(key); };
      bool remove(const char* key) override { return _prefs.remove(key); }
      void removeAll() override { _prefs.clear(); }
      bool store(const char* key, const Config::Value& value) override {
        return std::visit([&](auto&& v) -> bool {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, const char*>)
            return _prefs.putString(key, v);
          return false;
        },
                          value);
      }
      std::optional<Config::Value> load(const char* key) const override {
        switch (_prefs.getType(key)) {
          case PT_STR: {
            String s = _prefs.getString(key);
            char* buffer = new char[s.length() + 1];
            strcpy(buffer, s.c_str()); // NOLINT
            return Config::Value(std::unique_ptr<char[], void (*)(char[])>(buffer, _deleter_delete));
          }
          default:
            return std::nullopt;
        }
      }

    private:
      mutable Preferences _prefs;
      static void _deleter_delete(char p[]) { delete[] p; }
  };
} // namespace Mycila
