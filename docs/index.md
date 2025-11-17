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

- 💾 **Persistent storage** using ESP32 NVS with pluggable storage backend
- 🎯 **Default values** with efficient memory management (flash strings stored as pointers)
- 🔢 **Typed getters**: `getString()`, `getBool()`, `getInt()`, `getLong()`, `getFloat()`, `getI64()`, and more
- 🔔 **Change listener** callback fired when values change
- 🔄 **Restore callback** fired after bulk configuration restore
- ✅ **Validators** can be set globally, per-key, or during key configuration
- 💿 **Backup and restore** from key=value text format or `std::map`
- 📋 **Optional JSON export** with ArduinoJson integration (`toJson()`)
- 🔐 **Password masking** for keys ending with `_pwd` in JSON output
- 🎚️ **Smart restore order**: non-`_enable` keys applied first, then `_enable` keys last (useful for feature toggles)
- 🔑 **Key helpers**: `isPasswordKey()`, `isEnableKey()` detect special suffixes
- 📏 **NVS constraint enforcement**: asserts key names ≤ 15 characters
- 🧠 **Memory tracking**: `heapUsage()` method reports memory consumption

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

  // Declare configuration keys with optional default values
  // Key names must be ≤ 15 characters
  config.configure("debug_enable", "false");
  config.configure("wifi_ssid");
  config.configure("wifi_pwd");
  config.configure("port", "80");
  
  // Initialize the config system with NVS namespace
  config.begin("CONFIG");
  
  // Register change callback
  config.listen([](const char* key, const char* newValue) {
    Serial.printf("Config changed: %s = %s\n", key, newValue);
  });

  // Listen to configuration restore events
  config.listen([]() {
    Serial.println("Configuration restored!");
  });

  // Set a global validator (optional)
  config.setValidator([](const char* key, const char* value) {
    // Example: limit all values to 64 characters
    return strlen(value) <= 64;
  });

  // Set a per-key validator with configure()
  config.configure("port", "80", [](const char* key, const char* value) {
    int port = atoi(value);
    return port > 0 && port < 65536;
  });

  // Set configuration values
  auto result = config.setString("wifi_ssid", "MyNetwork");
  if (result) {
    Serial.println("WiFi SSID saved successfully");
    if (result.isStorageUpdated()) {
      Serial.println("Value was written to NVS storage");
    }
  } else {
    // Check detailed result
    switch ((Mycila::Config::Status)result) {
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
  Serial.printf("SSID: %s\n", config.getString("wifi_ssid"));
  bool debug = config.getBool("debug_enable");
  int port = config.getInt("port");

  // Check if value is empty or equals something
  if (config.isEmpty("wifi_pwd")) {
    Serial.println("Password not set");
  }
  if (config.isEqual("debug_enable", "true")) {
    Serial.println("Debug mode enabled");
  }
  
  // Check memory usage
  Serial.printf("Config heap usage: %d bytes\n", config.heapUsage());
}

void loop() {}
```

## API Reference

### Class: `Mycila::Config`

#### Setup and Storage

- **`bool begin(const char* name = "CONFIG", bool preload = false)`**  
  Initialize the configuration system with the specified NVS namespace. Returns true on success.
  - `preload = false` (default): Values are loaded from NVS on-demand when first accessed (lazy loading)
  - `preload = true`: All stored values are loaded into cache immediately during initialization
  
  **Preloading benefits:**
  - Faster subsequent access (all values already in cache)
  - Useful when you need to access many config values during startup
  - Increases initial memory usage but reduces NVS read operations
  
  **Example:**
  ```cpp
  // Lazy loading (default) - minimal startup time
  config.begin("CONFIG");
  
  // Preload all values - faster access, higher initial memory
  config.begin("CONFIG", true);
  ```

- **`bool configure(const char* key, const std::string& defaultValue, ConfigValidatorCallback validator = nullptr)`**  
- **`bool configure(const char* key, const char* defaultValue = "", ConfigValidatorCallback validator = nullptr)`**  
  Register a configuration key with an optional default value and validator. Key must be ≤ 15 characters. Returns false if the key is invalid or already exists.
  
  **Default value handling:**
  - `const char*`: Stored as pointer (zero copy if in flash/ROM)
  - `std::string`: Copied to heap and managed automatically

- **`bool configured(const char* key) const`**  
  Check if a configuration key has been registered.

- **`bool stored(const char* key) const`**  
  Check if a configuration key is currently stored in NVS (not just using default).

- **`void clear()`**  
  Clear all persisted settings from NVS and cache.

- **`size_t heapUsage() const`**  
  Returns the total heap memory consumed by the config system

#### Reading Values

- **`const char* getString(const char* key) const`**  
  Get the value as a C-string. Returns `nullptr` for unknown keys, otherwise returns the value (stored or default).

- **`bool getBool(const char* key) const`**  
  Parse value as boolean. Returns true for:
  - `MYCILA_CONFIG_VALUE_TRUE` (default: `"true"`)
  - If `MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING` is enabled (default): `"1"`, `"on"`, `"yes"`

- **`int getInt(const char* key) const`**  
  Parse value as integer (internally uses `getI32()` with `std::stol()`).

- **`long getLong(const char* key) const`**  
  Parse value as long using `std::stol()`.

- **`int64_t getI64(const char* key) const`**  
  Parse value as 64-bit integer using `std::stoll()`.

- **`float getFloat(const char* key) const`**  
  Parse value as float using `std::stof()`.

- **`bool isEmpty(const char* key) const`**  
  Check if the value is an empty string.

- **`bool isEqual(const char* key, const std::string& value) const`**  
- **`bool isEqual(const char* key, const char* value) const`**  
  Compare the stored value with the provided value.

#### Writing Values

- **`Result setString(const char* key, const std::string& value, bool fireChangeCallback = true)`**  
- **`Result setString(const char* key, const char* value, bool fireChangeCallback = true)`**  
  Set a configuration value. Returns an `Result` that converts to `bool` (true = operation successful).

- **`bool set(const std::map<const char*, std::string>& settings, bool fireChangeCallback = true)`**  
  Set multiple values at once. Returns true if any value was changed.

- **`bool setBool(const char* key, bool value, bool fireChangeCallback = true)`**  
  Set a boolean value (stored as `MYCILA_CONFIG_VALUE_TRUE` or `MYCILA_CONFIG_VALUE_FALSE`).

- **`Result unset(const char* key, bool fireChangeCallback = true)`**  
  Remove the persisted value (revert to default). Returns an `Result` indicating success or error.

#### Result and Status Enum

`set()` and `unset()` return a `Result` object that:

- Converts to `bool` (true if operation was successful)
- Can be cast to `Mycila::Config::Status` enum for detailed status
- Has `isStorageUpdated()` method (true only if NVS storage was actually modified)

**Status codes:**

**Success codes** (converts to `true`):
- `PERSISTED` — Value changed and written to NVS
- `DEFAULTED` — Value equals default, removed from NVS or not stored
- `REMOVED` — Key successfully removed from NVS

**Error codes** (converts to `false`):
- `ERR_UNKNOWN_KEY` — Key not registered via `configure()`
- `ERR_INVALID_VALUE` — Rejected by validator
- `ERR_FAIL_ON_WRITE` — NVS write operation failed
- `ERR_FAIL_ON_REMOVE` — NVS remove operation failed

**Example:**

```cpp
auto res = config.setString("key", "value");

// Simple success check
if (res) {
  Serial.println("Success!");
}

// Check if storage was updated
if (res.isStorageUpdated()) {
  Serial.println("Value written to NVS");
}

// Detailed error handling
if (!res) {
  switch ((Mycila::Config::Status)res) {
    case Mycila::Config::Status::ERR_INVALID_VALUE:
      Serial.println("Validator rejected the value");
      break;
    case Mycila::Config::Status::ERR_UNKNOWN_KEY:
      Serial.println("Key not configured");
      break;
    case Mycila::Config::Status::ERR_FAIL_ON_WRITE:
      Serial.println("NVS write failed");
      break;
    default:
      break;
  }
}

// You can also return Status directly from functions returning Result:
Mycila::Config::Result myFunction() {
  if (error) {
    return Mycila::Config::Status::ERR_UNKNOWN_KEY;
  }
  return config.setString("key", "value");
}
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
  Set a per-key validator. Pass `nullptr` to remove. Returns false if key doesn't exist.

**Note:** Validators can also be set during `configure()`:
```cpp
config.configure("port", "80", [](const char* key, const char* value) {
  int port = atoi(value);
  return port > 0 && port < 65536;
});
```

#### Backup and Restore

- **`void backup(Print& out, bool includeDefaults = true)`**  
  Write configuration as `key=value\n` lines to any `Print` destination (Serial, File, etc.).
  - `includeDefaults = true`: Exports all keys (even those using defaults)
  - `includeDefaults = false`: Only exports keys with stored values

- **`bool restore(const char* data)`**  
  Parse and restore configuration from `key=value\n` formatted text. Returns true if successful.

- **`bool restore(const std::map<const char*, std::string>& settings)`**  
  Restore configuration from a map. Returns true if successful.

**Restore order:** Non-`_enable` keys are applied first, then `_enable` keys, ensuring feature toggles are activated after their dependencies.

#### Utilities

- **`const std::vector<const char*>& keys() const`**  
  Get a sorted vector of all registered configuration key names.

- **`const char* keyRef(const char* buffer) const`**  
  Given a string buffer, return the canonical key pointer if it matches a registered key (useful for pointer comparisons). Returns `nullptr` if not found.

- **`bool isPasswordKey(const char* key) const`**  
  Returns true if key ends with `MYCILA_CONFIG_KEY_PASSWORD_SUFFIX` (default: `"_pwd"`).

- **`bool isEnableKey(const char* key) const`**  
  Returns true if key ends with `MYCILA_CONFIG_KEY_ENABLE_SUFFIX` (default: `"_enable"`).

- **`void toJson(const JsonObject& root) const`** *(requires `MYCILA_JSON_SUPPORT`)*  
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
// In platformio.ini
build_flags =
  -D MYCILA_CONFIG_PASSWORD_MASK=\"****\"
```

## Backup and Restore Example

```cpp
// Backup to Serial
config.backup(Serial);

// Backup to String (all keys including defaults)
String backup;
StringPrint sp(backup);
config.backup(sp, true);
Serial.println(backup);

// Backup to String (only stored values)
String backupStored;
StringPrint sp2(backupStored);
config.backup(sp2, false);

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

## Configuration Defines

Customize behavior with build flags:

```cpp
// Key suffix for password keys (default: "_pwd")
-D MYCILA_CONFIG_KEY_PASSWORD_SUFFIX=\"_password\"

// Key suffix for enable keys (default: "_enable")
-D MYCILA_CONFIG_KEY_ENABLE_SUFFIX=\"_on\"

// Show passwords in JSON export (default: masked)
-D MYCILA_CONFIG_SHOW_PASSWORD

// Password mask string (default: "********")
-D MYCILA_CONFIG_PASSWORD_MASK=\"[REDACTED]\"

// Boolean value strings (defaults: "true" / "false")
-D MYCILA_CONFIG_VALUE_TRUE=\"1\"
-D MYCILA_CONFIG_VALUE_FALSE=\"0\"

// Enable extended bool parsing: yes/no, on/off, etc. (default: 1)
-D MYCILA_CONFIG_EXTENDED_BOOL_VALUE_PARSING=0
```

## Key Naming Conventions

- **Maximum length:** 15 characters (NVS limitation enforced by assert)
- **Password keys:** End with `_pwd` (or custom suffix) → masked in JSON export
- **Enable keys:** End with `_enable` (or custom suffix) → applied last during bulk restore

## Memory Optimization

The library optimizes memory usage through:

1. **Zero-copy flash strings**: Default values that are string literals stored in ROM/DROM are referenced by pointer, consuming zero heap.

2. **Smart caching**: Values are cached on first access, avoiding repeated NVS reads.

3. **RAII memory management**: The `Str` class automatically manages heap-allocated strings with move semantics.

4. **Heap tracking**: Use `heapUsage()` to monitor exact memory consumption:

```cpp
Serial.printf("Config heap usage: %d bytes\n", config.heapUsage());
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

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

### Large Configuration
See [`examples/Big/Big.ino`](examples/Big/Big.ino) for:
- Managing 150+ configuration keys
- Heap usage monitoring
- Random operations stress test
- Performance benchmarking

## Custom Storage Backend

You can implement custom storage backends by inheriting from `Config::Storage`:

```cpp
class MyCustomStorage : public Mycila::Config::Storage {
  public:
    bool begin(const char* name) override {
      // Initialize your storage
      return true;
    }
    
    bool hasKey(const char* key) const override {
      // Check if key exists
      return false;
    }
    
    std::optional<Mycila::Config::Str> loadString(const char* key) const override {
      // Load string value from your storage
      // Return std::nullopt if key doesn't exist
      return std::nullopt;
    }
    
    bool storeString(const char* key, const char* value) override {
      // Save string value to your storage
      return true;
    }
    
    bool remove(const char* key) override {
      // Remove key from your storage
      return true;
    }
    
    bool removeAll() override {
      // Clear all keys
      return true;
    }
    
    // Optional: Implement typed load/store methods for better performance
    std::optional<int32_t> loadI32(const char* key) const override { return std::nullopt; }
    bool storeI32(const char* key, int32_t value) override { return false; }
    // ... other typed methods
};

// Usage
MyCustomStorage storage;
Mycila::Config config(storage);
config.begin("MYAPP");
```

For a complete reference implementation, see [`src/MycilaConfigStorageNVS.h`](src/MycilaConfigStorageNVS.h).

## License

MIT License - see [LICENSE](LICENSE) file for details.
