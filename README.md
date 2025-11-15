# MycilaConfig

[![Latest Release](https://img.shields.io/github/release/mathieucarbou/MycilaConfig.svg)](https://GitHub.com/mathieucarbou/MycilaConfig/releases/)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/MycilaConfig.svg)](https://registry.platformio.org/libraries/mathieucarbou/MycilaConfig)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)

[![Build](https://github.com/mathieucarbou/MycilaConfig/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/MycilaConfig/actions/workflows/ci.yml)
[![GitHub latest commit](https://badgen.net/github/last-commit/mathieucarbou/MycilaConfig)](https://GitHub.com/mathieucarbou/MycilaConfig/commit/)
[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-Ready--to--Code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/mathieucarbou/MycilaConfig)

A simple, efficient configuration library for ESP32 (Arduino framework) built on top of Preferences (NVS). Provides defaults, caching, typed getters, validators, change/restore callbacks, backup/restore, and optional JSON export with password masking.

## Features

- 💾 **Persistent storage** using ESP32 Preferences (NVS)
- 🎯 **Default values** with transparent caching for fast access
- 🔢 **Typed getters**: `getString()`, `getBool()`, `getInt()`, `getLong()`, `getFloat()`
- 🔔 **Change listener** callback fired when values change
- 🔄 **Restore callback** fired after bulk configuration restore
- ✅ **Global and per-key validators** to enforce value constraints before persisting
- 💿 **Backup and restore** from key=value text format or `std::map`
- 📋 **Optional JSON export** with ArduinoJson integration (`toJson()`)
- 🔐 **Password masking** for keys ending with `_pwd` in JSON output
- 🎚️ **Smart restore order**: non-`_enable` keys applied first, then `_enable` keys last (useful for feature toggles)
- 🔑 **Key helpers**: `isPasswordKey()`, `isEnableKey()` detect special suffixes
- 📏 **NVS constraint enforcement**: asserts key names ≤ 15 characters

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
  mathieucarbou/MycilaConfig
```

### Optional: JSON Support

To enable `toJson()` method with ArduinoJson:

```ini
build_flags =
  -D MYCILA_JSON_SUPPORT
lib_deps =
  mathieucarbou/MycilaConfig
  bblanchon/ArduinoJson
```

## Quick Start

```cpp
#include <MycilaConfig.h>

Mycila::Config config;

void setup() {
  Serial.begin(115200);
  
  // Initialize the config system with NVS namespace
  config.begin("CONFIG");

  // Declare configuration keys with optional default values
  // Key names must be ≤ 15 characters
  config.configure("debug_enable", MYCILA_CONFIG_VALUE_FALSE);
  config.configure("wifi_ssid");
  config.configure("wifi_pwd");
  config.configure("port", "80");

  // Register change callback
  config.listen([](const char* key, const char* newValue) {
    Serial.printf("Config changed: %s = %s\n", key, newValue);
  });

  // Listen to configuration restore events
  config.listen([]() {
    Serial.println("Configuration restored!");
  });

  // Set a global validator (optional)
  config.setValidator([](const char* key, const std::string& value) {
    // Example: limit all values to 64 characters
    return value.size() <= 64;
  });

  // Set a per-key validator (optional)
  config.setValidator("port", [](const char* key, const char* value) {
    int port = std::stoi(value);
    return port > 0 && port < 65536;
  });

  // Set configuration values
  auto status = config.set("wifi_ssid", "MyNetwork");
  if (status) {
    Serial.println("WiFi SSID saved successfully");
    if (status.isStorageUpdated()) {
      Serial.println("Value was written to storage");
    }
  } else {
    // Check detailed status
    switch ((Mycila::Config::Status)status) {
      case Mycila::Config::Status::ERR_INVALID_VALUE:
        Serial.println("Invalid value rejected by validator");
        break;
      case Mycila::Config::Status::ERR_UNKNOWN_KEY:
        Serial.println("Key not configured");
        break;
      default:
        break;
    }
  }

  // Get configuration values
  Serial.printf("SSID: %s\n", config.get("wifi_ssid"));
  bool debug = config.getBool("debug_enable");
  int port = config.getInt("port");

  // Check if value is empty or equals something
  if (config.isEmpty("wifi_pwd")) {
    Serial.println("Password not set");
  }
  if (config.isEqual("debug_enable", MYCILA_CONFIG_VALUE_TRUE)) {
    Serial.println("Debug mode enabled");
  }
}

void loop() {}
```

## API Reference

### Class: `Mycila::Config`

#### Setup and Storage

- **`void begin(const char* name = "CONFIG")`**  
  Initialize the configuration system with the specified NVS namespace.

- **`bool configure(const char* key, const std::string& defaultValue)`**  
- **`bool configure(const char* key, const char* defaultValue = "")`**  
  Register a configuration key with an optional default value. Key must be ≤ 15 characters. Returns true if the key was added successfully.

- **`bool exists(const char* key) const`**  
  Check if a configuration key has been registered.

- **`bool stored(const char* key) const`**  
  Check if a configuration key is currently stored in NVS (not just using default).

- **`void clear()`**  
  Clear all persisted settings and cache.

#### Reading Values

- **`const char* get(const char* key) const`**  
  Get the value as a C-string (never returns `nullptr`, returns `""` for unknown keys).

- **`std::string getString(const char* key) const`**  
  Get the value as a `std::string` (returns a copy).

- **`bool getBool(const char* key) const`**  
  Parse value as boolean:
  - If `-D MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING=1` (or not defined): (`MYCILA_CONFIG_VALUE_TRUE`, `"true"`, `"1"`, `"on"`, `"yes"`) → `true`. **This is the default behavior.**
  - If `-D MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING=0`: only `MYCILA_CONFIG_VALUE_TRUE` → `true`

- **`int getInt(const char* key) const`**  
  Parse value as integer using `std::stoi()`.

- **`long getLong(const char* key) const`**  
  Parse value as long using `std::stol()`.

- **`float getFloat(const char* key) const`**  
  Parse value as float using `std::stof()`.

- **`bool isEmpty(const char* key) const`**  
  Check if the value is an empty string.

- **`bool isEqual(const char* key, const std::string& value) const`**  
- **`bool isEqual(const char* key, const char* value) const`**  
  Compare the stored value with the provided value.

#### Writing Values

- **`Result set(const char* key, const std::string& value, bool fireChangeCallback = true)`**  
- **`Result set(const char* key, const char* value, bool fireChangeCallback = true)`**  
  Set a configuration value. Returns an `Result` that converts to `bool` (true = operation successful).

- **`bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true)`**  
  Set multiple values at once. Returns true if any value was changed.

- **`bool setBool(const char* key, bool value)`**  
  Set a boolean value (stored as `MYCILA_CONFIG_VALUE_TRUE` or `MYCILA_CONFIG_VALUE_FALSE`).
  Eg., `MYCILA_CONFIG_VALUE_TRUE` = "true", `MYCILA_CONFIG_VALUE_FALSE` = "false" (by default).

- **`Result unset(const char* key, bool fireChangeCallback = true)`**  
  Remove the persisted value (revert to default). Returns an `Result` indicating success or error.

#### Result and Status Enum

`set()` and `unset()` return an `Result` object that:

- Converts to `bool` (true if operation was successful)
- Can be cast to `Mycila::Config::Status` enum for detailed status
- Has `isStorageUpdated()` method (true only if storage was actually modified)

**Status codes:**

- `PERSISTED` — Value written to NVS successfully
- `DEFAULTED` — Value matches default and key not persisted (no-op)
- `REMOVED` — Key successfully removed from NVS, or was not present (no-op)
- `ERR_UNKNOWN_KEY` — Key not registered via `configure()`
- `ERR_INVALID_VALUE` — Rejected by validator
- `ERR_FAIL_ON_WRITE` — NVS write failed
- `ERR_FAIL_ON_REMOVE` — NVS remove failed

**Example:**

```cpp
auto res = config.set("key", "value");
if (!res) {
  switch ((Mycila::Config::Status)res) {
    case Mycila::Config::Status::ERR_INVALID_VALUE:
      Serial.println("Value rejected by validator");
      break;
    case Mycila::Config::Status::ERR_UNKNOWN_KEY:
      Serial.println("Key not configured");
      break;
    default:
      break;
  }
}

// Check if storage was actually updated
if (res.isStorageUpdated()) {
  Serial.println("Value was written to NVS");
}

// You can also return Status directly from functions returning Result:
// return Mycila::Config::Status::ERR_UNKNOWN_KEY;
```

#### Callbacks and Validators

- **`void listen(ConfigChangeCallback callback)`**  
  Register a callback invoked when any value changes: `void callback(const char* key, const char* newValue)`

- **`void listen(ConfigRestoredCallback callback)`**  
  Register a callback invoked after a bulk restore: `void callback()`

- **`bool setValidator(ConfigValidatorCallback callback)`**  
  Set a global validator called for all keys. Pass `nullptr` to remove.  
  Signature: `bool validator(const char* key, const char* newValue)`

- **`bool setValidator(const char* key, ConfigValidatorCallback callback)`**  
  Set a per-key validator. Pass `nullptr` to remove.

#### Backup and Restore

- **`void backup(Print& out)`**  
  Write all configuration as `key=value\n` lines to any `Print` destination (Serial, StreamString, etc.).

- **`bool restore(const char* data)`**  
  Parse and restore configuration from `key=value\n` formatted text.

- **`bool restore(const std::map<const char*, std::string>& settings)`**  
  Restore configuration from a map. Returns true if any value changed.

**Restore order:** Non-`_enable` keys are applied first, then `_enable` keys, ensuring feature toggles are activated after their dependencies.

#### Utilities

- **`const std::vector<const char*>& keys() const`**  
  Get a sorted list of all registered configuration keys.

- **`const char* keyRef(const char* buffer) const`**  
  Given a string buffer, return the canonical key pointer if it matches a registered key (useful for pointer comparisons).

- **`bool isPasswordKey(const char* key) const`**  
  Returns true if key ends with `_pwd`.

- **`bool isEnableKey(const char* key) const`**  
  Returns true if key ends with `_enable`.

- **`void toJson(const JsonObject& root)`** *(requires `MYCILA_JSON_SUPPORT`)*  
  Export all configuration to an ArduinoJson object. Password keys are masked.

## JSON Export and Password Masking

When `MYCILA_JSON_SUPPORT` is defined, you can export configuration to JSON:

```cpp
#include <ArduinoJson.h>

JsonDocument doc;
config.toJson(doc.to<JsonObject>());
serializeJson(doc, Serial);
```

Keys ending with `_pwd` are automatically masked in the JSON output (unless you define `MYCILA_CONFIG_SHOW_PASSWORD`).

**Customize the mask:**
```cpp
#define MYCILA_CONFIG_PASSWORD_MASK "****"  // default is "********"
```

## Backup and Restore Example

```cpp
// Backup to StreamString
StreamString backup;
backup.reserve(1024);
config.backup(backup);
Serial.println(backup);

// Restore from text
const char* savedConfig = 
  "wifi_ssid=MyNetwork\n"
  "wifi_pwd=secret123\n"
  "debug_enable=true\n";
config.restore(savedConfig);

// Restore from map
std::map<const char*, std::string> settings = {
  {"wifi_ssid", "NewNetwork"},
  {"port", "8080"}
};
config.restore(settings);
```

## Key Naming Conventions

- **Maximum length:** 15 characters (NVS limitation enforced by assert)
- **Password keys:** End with `_pwd` → masked in JSON export
- **Enable keys:** End with `_enable` → applied last during bulk restore

## Examples

### Basic Configuration
See [`examples/Config/Config.ino`](examples/Config/Config.ino) for a complete example demonstrating:
- Setting up keys and defaults
- Using validators
- Setting and getting values
- Checking Result codes
- Backup and restore

### JSON Export
See [`examples/ConfigJson/ConfigJson.ino`](examples/ConfigJson/ConfigJson.ino) for:
- JSON export with `toJson()`
- Password masking
- Integration with ArduinoJson

## License

MIT License - see [LICENSE](LICENSE) file for details.
