# MycilaConfig

[![Latest Release](https://img.shields.io/github/release/mathieucarbou/MycilaConfig.svg)](https://GitHub.com/mathieucarbou/MycilaConfig/releases/)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/MycilaConfig.svg)](https://registry.platformio.org/libraries/mathieucarbou/MycilaConfig)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)

[![Build](https://github.com/mathieucarbou/MycilaConfig/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/MycilaConfig/actions/workflows/ci.yml)
[![GitHub latest commit](https://badgen.net/github/last-commit/mathieucarbou/MycilaConfig)](https://GitHub.com/mathieucarbou/MycilaConfig/commit/)
[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-Ready--to--Code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/mathieucarbou/MycilaConfig)

A simple, efficient configuration library for ESP32 (Arduino framework) with pluggable storage backends (NVS included). Supports native types (bool, integers, floats, strings) with type safety via `std::variant`. Provides defaults, caching, generic typed API, validators, change/restore callbacks, backup/restore, and optional JSON export with password masking.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [PlatformIO](#platformio)
  - [Optional: JSON Support](#optional-json-support)
- [Quick Start](#quick-start)
- [Migrating from v10 to v11](#migrating-from-v10-to-v11)
- [API Reference](#api-reference)
  - [Class: `Mycila::config::Config`](#class-mycilaconfigconfig)
    - [Constructor](#constructor)
    - [Setup and Storage](#setup-and-storage)
    - [Reading Values](#reading-values)
    - [Writing Values](#writing-values)
    - [Result and Status Enum](#result-and-status-enum)
    - [Callbacks and Validators](#callbacks-and-validators)
    - [Backup and Restore](#backup-and-restore)
    - [Utilities](#utilities)
- [JSON Export and Password Masking](#json-export-and-password-masking)
- [Backup and Restore Example](#backup-and-restore-example)
- [Configuration Defines](#configuration-defines)
- [Key Naming Conventions](#key-naming-conventions)
- [Memory Optimization](#memory-optimization)
- [Examples](#examples)
  - [Test Example](#test-example)
  - [JSON Export](#json-export)
  - [Native Type Support](#native-type-support)
  - [Large Configuration](#large-configuration)
- [Custom Storage Backend](#custom-storage-backend)
- [License](#license)

## Features

- 💾 **Persistent storage** using ESP32 NVS with pluggable storage backend
- 🎯 **Default values** with efficient memory management (flash strings stored as pointers)
- 🔢 **Native type support**: bool, int8/16/32/64, uint8/16/32/64, int, unsigned int, float, double (optional), and strings via `std::variant`
- ⚡ **Generic typed API**: `get<T>()` and `set<T>()` with compile-time type safety
- 🔔 **Change listener** callback fired when values change with variant values
- 🔄 **Restore callback** fired after bulk configuration restore
- ✅ **Validators** can be set globally, per-key, or during key configuration (receive variant values)
- 💿 **Backup and restore** from key=value text format or `std::map<const char*, Value>`
- 📋 **Optional JSON export** with ArduinoJson integration (`toJson()`) - native types exported directly
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
#include <MycilaConfigStorageNVS.h>

Mycila::config::NVS storage;
Mycila::config::Config config(storage);

void setup() {
  Serial.begin(115200);

  // Declare configuration keys with optional default values
  // Key names must be ≤ 15 characters
  config.configure("debug_enable", false);
  config.configure("wifi_ssid", "MyNetwork");
  config.configure("wifi_pwd", "");
  config.configure("port", 80);
  config.configure("timeout", 30.0f);

  config.begin("MYAPP", true); // Preload all values

  // Use typed getters/setters
  bool debug = config.get<bool>("debug_enable");
  int port = config.get<int>("port");
  float timeout = config.get<float>("timeout");
  const char* ssid = config.getString("wifi_ssid");
}
```

## Migrating from v10 to v11

Version 11 introduces a major refactoring with:

- New namespace structure: `Mycila::config::`
- Native type support with `std::variant`
- Generic typed API: `get<T>()` and `set<T>()`
- Callbacks now receive `std::optional<Value>` instead of string values

To ease migration from v10, a **deprecated compatibility wrapper** is provided that maintains the v10 string-based API:

```cpp
#include <MycilaConfig.h>
#include <MycilaConfigV10.h>
#include <MycilaConfigStorageNVS.h>

Mycila::config::NVS storage;
Mycila::config::Config configNew(storage);
Mycila::config::ConfigV10 config(configNew);

void setup() {
  // Use the old v10 API - all methods still work
  config.configure("debug_enable", "false");
  config.configure("port", "80");
  
  config.begin("MYAPP");
  
  // Old string-based API
  const char* port = config.getString("port");
  bool debug = config.getBool("debug_enable");
  int portNum = config.getInt("port");
  
  // Old string-based callbacks
  config.listen([](const char* key, const char* newValue) {
    Serial.printf("Changed: %s = %s\n", key, newValue);
  });
  
  // Old validators
  config.setValidator("port", [](const char* key, const char* value) {
    int p = atoi(value);
    return p > 0 && p < 65536;
  });
  
  config.setString("port", "8080");
}
```

**Migration path:**

1. **Immediate compatibility**: Include `MycilaConfigV10.h` and use `ConfigV10` wrapper - no code changes needed
2. **Gradual migration**: Start using new typed API alongside deprecated API
3. **Full migration**: Remove deprecated wrapper and update to new API

**Note:** The deprecated wrapper will be removed in a future major version. Plan to migrate to the new typed API.

## API Reference

### Class: `Mycila::config::Config`

#### Constructor

- **`Config(Storage& storage)`**  
  Create a Config instance with the specified storage backend. The storage reference must remain valid for the lifetime of the Config object.

  **Example:**

  ```cpp
  Mycila::config::NVS storage;
  Mycila::config::Config config(storage);
  ```

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

- **`template <typename T = Value> bool configure(const char* key, T defaultValue, ValidatorCallback validator = nullptr)`**
- **`bool configure(const char* key, ValidatorCallback validator = nullptr)`** (defaults to empty string)  
  Register a configuration key with an optional default value and validator. Key must be ≤ 15 characters. Returns true if the key was successfully registered, false otherwise (e.g., key too long).

  **Supported types for `T`:**

  - `bool` - Boolean values
  - `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t` - Fixed-width integers
  - `int`, `unsigned int` - Standard integers
  - `int64_t`, `uint64_t` - 64-bit integers (if `MYCILA_CONFIG_USE_LONG_LONG` enabled)
  - `float` - Single precision floating point
  - `double` - Double precision (if `MYCILA_CONFIG_USE_DOUBLE` enabled)
  - `const char*` - C-strings (stored as pointer if in flash/ROM, zero copy)
  - `Mycila::config::Str` - String wrapper with heap/flash detection

  **Example:**

  ```cpp
  config.configure("enabled", false);
  config.configure("port", 8080);
  config.configure("threshold", 25.5f);
  config.configure("name", "Device");
  config.configure("count", static_cast<uint32_t>(1000));
  ```

- **`bool configured(const char* key) const`**  
  Check if a configuration key has been registered.

- **`bool stored(const char* key) const`**  
  Check if a configuration key is currently stored in NVS (not just using default).

- **`void clear()`**  
  Clear all persisted settings from NVS and cache.

- **`size_t heapUsage() const`**  
  Returns the total heap memory consumed by the config system, including:
  - Vector storage for Key objects (capacity × sizeof(Key))
  - Heap usage from default values embedded in Key objects
  - Map structure overhead (red-black tree nodes) for cache and validators
  - Str object structures in maps
  - Heap-allocated string content (flash strings contribute 0 bytes)

#### Reading Values

- **`template <typename T = Value> const T& get(const char* key) const`**  
  Get the typed value from configuration. Returns a reference to the value with the specified type. Throws `std::runtime_error` if the key doesn't exist or type doesn't match.

  **Supported types:**

  - `bool`, `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int`, `unsigned int`
  - `int64_t`, `uint64_t` (if `MYCILA_CONFIG_USE_LONG_LONG` enabled)
  - `float`, `double` (if `MYCILA_CONFIG_USE_DOUBLE` enabled)
  - `Mycila::config::Str` - Returns string wrapper

  **Example:**

  ```cpp
  bool enabled = config.get<bool>("debug_enable");
  int port = config.get<int>("port");
  float temp = config.get<float>("temperature");
  uint32_t count = config.get<uint32_t>("counter");
  ```

- **`const char* getString(const char* key) const`**  
  Get the value as a C-string. Convenience wrapper around `get<Str>(key).c_str()`.

  **Example:**

  ```cpp
  const char* ssid = config.getString("wifi_ssid");
  ```

- **`bool isEmpty(const char* key) const`**  
  Check if the string value is empty.

- **`bool isEqual(const char* key, const std::string& value) const`**
- **`bool isEqual(const char* key, const char* value) const`**  
  Compare the stored string value with the provided value.

#### Writing Values

- **`template <typename T = Value> const Result set(const char* key, T value, bool fireChangeCallback = true)`**  
  Set a typed configuration value. Returns a `Result` object that converts to `bool` (true = operation successful). The type must match the type used during `configure()`.

  **Supported types:** Same as `get<T>()`

  **Example:**

  ```cpp
  config.set<bool>("debug_enable", true);
  config.set<int>("port", 8080);
  config.set<float>("threshold", 25.5f);
  config.set<uint32_t>("counter", 1000);
  ```

- **`Result setString(const char* key, const std::string& value, bool fireChangeCallback = true)`**
- **`Result setString(const char* key, const char* value, bool fireChangeCallback = true)`**  
  Set a string configuration value. Convenience wrapper around `set<Str>()`.

  **Example:**

  ```cpp
  config.setString("wifi_ssid", "MyNetwork");
  ```

- **`bool set(std::map<const char*, Value> settings, bool fireChangeCallback = true)`**  
  Set multiple values at once from a map of variant values. Returns true if any value was changed. Pass map by value or use `std::move()` to avoid copying.

  **Example:**

  ```cpp
  std::map<const char*, Mycila::config::Value> batch;
  batch.emplace("enabled", true);
  batch.emplace("port", 8080);
  batch.emplace("name", Mycila::config::Str("Device"));
  config.set(std::move(batch));
  ```

- **`Result unset(const char* key, bool fireChangeCallback = true)`**  
  Remove the persisted value (revert to default). Returns a `Result` indicating success or error.

#### Result and Status Enum

`set()` and `unset()` return a `Result` object that:

- Converts to `bool` (true if operation was successful)
- Can be cast to `Mycila::config::Status` enum for detailed status
- Has `isStorageUpdated()` method (true only if NVS storage was actually modified)

**Status codes:**

**Success codes** (converts to `true`):

- `PERSISTED` — Value changed and written to NVS
- `DEFAULTED` — Value equals default, not stored
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
  switch ((Mycila::config::Status)res) {
    case Mycila::config::Status::ERR_INVALID_VALUE:
      Serial.println("Validator rejected the value");
      break;
    case Mycila::config::Status::ERR_UNKNOWN_KEY:
      Serial.println("Key not configured");
      break;
    case Mycila::config::Status::ERR_FAIL_ON_WRITE:
      Serial.println("NVS write failed");
      break;
    default:
      break;
  }
}

// You can also return Status directly from functions returning Result:
Mycila::config::Result myFunction() {
  if (error) {
    return Mycila::config::Status::ERR_UNKNOWN_KEY;
  }
  return config.setString("key", "value");
}
```

#### Callbacks and Validators

- **`void listen(ChangeCallback callback)`**  
  Register a callback invoked when any value changes.  
  **Signature:** `void callback(const char* key, const std::optional<Mycila::config::Value>& newValue)`

  The callback receives a `std::optional<std::variant>` value:

  - `newValue.has_value() == true` - Key was set or updated with a typed value
  - `newValue.has_value() == false` (std::nullopt) - Key was removed/unset

  ```cpp
  config.listen([](const char* key, const Mycila::config::Value& newValue) {
      Serial.printf("Key '%s' changed to: %s\n", key, newValue.as<const char*>());

      // Type-specific handling
      if (std::holds_alternative<bool>(newValue.value())) {
        bool val = std::get<bool>(newValue.value());
        Serial.printf("Boolean value: %s\n", val ? "true" : "false");
      } else if (std::holds_alternative<int>(newValue.value())) {
        int val = std::get<int>(newValue.value());
        Serial.printf("Integer value: %d\n", val);
      }
    } else {
      Serial.printf("Key '%s' was removed/unset\n", key);
    }
  });
  ```

- **`void listen(RestoredCallback callback)`**  
  Register a callback invoked after a bulk restore: `void callback()`

- **`bool setValidator(ValidatorCallback callback)`**  
  Set a global validator called for all keys. Pass `nullptr` to remove.  
  **Signature:** `bool validator(const char* key, const Mycila::config::Value& newValue)`

- **`bool setValidator(const char* key, ValidatorCallback callback)`**  
  Set a per-key validator. Pass `nullptr` to remove. Returns false if key doesn't exist.

**Note:** Validators can also be set during `configure()` and receive typed variant values:

```cpp
config.configure("port", 80, [](const char* key, const Mycila::config::Value& value) {
  // Value is guaranteed to be int type (matches configure type)
  int port = std::get<int>(value);
  return port > 0 && port < 65536;
});

config.configure("temperature", 25.0f, [](const char* key, const Mycila::config::Value& value) {
  float temp = std::get<float>(value);
  return temp >= -40.0f && temp <= 85.0f;
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

- **`const std::vector<Key>& keys() const`**  
  Get a sorted vector of all registered configuration keys. Each `Key` contains `name` (const char\*) and `defaultValue` (Mycila::config::Value variant).

  **Example:**

  ```cpp
  for (const auto& key : config.keys()) {
    Serial.printf("%s = %s\n", key.name, key.defaultValue.toString().c_str());
  }
  ```

- **`const char* keyRef(const char* buffer) const`**  
  Given a string buffer, return the canonical key pointer if it matches a registered key (useful for pointer comparisons). Returns `nullptr` if not found.

- **`const Key* key(const char* buffer) const`**  
  Given a string buffer, return a pointer to the Key object if it matches a registered key. Returns `nullptr` if not found.

- **`bool isPasswordKey(const char* key) const`**  
  Returns true if key ends with `MYCILA_CONFIG_KEY_PASSWORD_SUFFIX` (default: `"_pwd"`).

- **`bool isEnableKey(const char* key) const`**  
  Returns true if key ends with `MYCILA_CONFIG_KEY_ENABLE_SUFFIX` (default: `"_enable"`).

- **`void toJson(const JsonObject& root) const`** _(requires `MYCILA_JSON_SUPPORT`)_  
  Export all configuration to an ArduinoJson object. Password keys are masked.

## JSON Export and Password Masking

When `MYCILA_JSON_SUPPORT` is defined, you can export configuration to JSON with **native type preservation**:

```cpp
#include <ArduinoJson.h>

JsonDocument doc;
config.toJson(doc.to<JsonObject>());
serializeJson(doc, Serial);
// Output: {"enabled":true,"port":8080,"threshold":25.5,"name":"Device"}
```

**Type handling in JSON:**

- Boolean values exported as JSON `true`/`false` (not strings)
- Integer values exported as JSON numbers
- Float/double values exported as JSON numbers with decimal points
- String values exported as JSON strings
- Password keys (ending with `_pwd`) are automatically masked as strings (unless you define `MYCILA_CONFIG_SHOW_PASSWORD`)

**Customize the mask:**

```cpp
// In platformio.ini
build_flags =
  -D MYCILA_CONFIG_PASSWORD_MASK=\"****\"
```

## Backup and Restore Example

````cpp
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

// Restore from map with typed values

```cpp
std::map<const char*, Mycila::config::Value> settings;
settings.emplace("wifi_ssid", Mycila::config::Str("NewNetwork"));
settings.emplace("port", 8080);
settings.emplace("enabled", true);
config.restore(std::move(settings));
````

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

// Enable 64-bit integer support (int64_t, uint64_t) (default: 1)
-D MYCILA_CONFIG_USE_LONG_LONG=1

// Enable double precision floating point support (default: 1)
-D MYCILA_CONFIG_USE_DOUBLE=1
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

### Test Example

See [`examples/Test/Test.ino`](examples/Test/Test.ino) for a complete example demonstrating:

- Setting up keys and defaults with string values
- Using global and per-key validators with variant values
- Setting and getting string values
- Checking Result codes and Status enum
- Backup and restore functionality
- Change and restore callbacks (with optional values for unset operations)

### JSON Export

See [`examples/Json/Json.ino`](examples/Json/Json.ino) for:

- JSON export with `toJson()`
- Backup to string with `backup()`
- Integration with ArduinoJson
- Toggling configuration values

### Native Type Support

See [`examples/Native/Native.ino`](examples/Native/Native.ino) for:

- Using all native types: bool, int8/16/32/64, uint8/16/32/64, int, unsigned int, float, double
- Type-safe getters and setters with `get<T>()` and `set<T>()`
- Validators with typed variant values
- Batch operations with native types
- Heap usage tracking

### Large Configuration

See [`examples/Big/Big.ino`](examples/Big/Big.ino) for:

- Managing 150+ configuration keys
- Heap usage monitoring
- Random operations stress test
- Performance benchmarking

## Custom Storage Backend

You can implement custom storage backends by inheriting from `Mycila::config::Storage`:

```cpp
class MyCustomStorage : public Mycila::config::Storage {
  public:
    bool begin(const char* name) override {
      // Initialize your storage
      return true;
    }

    bool hasKey(const char* key) const override {
      // Check if key exists
      return false;
    }

    std::optional<Mycila::config::Str> loadString(const char* key) const override {
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
    std::optional<bool> loadBool(const char* key) const override { return std::nullopt; }
    bool storeBool(const char* key, bool value) override { return false; }

    std::optional<int32_t> loadI32(const char* key) const override { return std::nullopt; }
    bool storeI32(const char* key, int32_t value) override { return false; }

    std::optional<float> loadFloat(const char* key) const override { return std::nullopt; }
    bool storeFloat(const char* key, float value) override { return false; }

    // ... other typed methods (loadI8/U8/I16/U16/I64/U64, loadDouble, etc.)
};

// Usage
MyCustomStorage storage;
Mycila::config::Config config(storage);
config.begin("MYAPP");
```

**Storage interface highlights:**

- All typed `load*()` methods return `std::optional<T>` (nullopt = key not found)
- All typed `store*()` methods return `bool` (true = success)
- The library automatically uses typed methods when available for better performance
- Falls back to `loadString()`/`storeString()` with conversion if typed methods not implemented

For a complete reference implementation, see the included NVS storage backend:

- Header: [`src/MycilaConfigStorageNVS.h`](src/MycilaConfigStorageNVS.h)
- Implementation: Inline in header file

## License

MIT License - see [LICENSE](LICENSE) file for details.
