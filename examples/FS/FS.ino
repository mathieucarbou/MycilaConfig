// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <Esp.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include <assert.h>

#include <MycilaConfig.h>
#include <MycilaConfigStorageFS.h>

Mycila::config::FileSystem storage;
Mycila::config::Config config(storage);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial.println("\n=== Native Type Test Example ===\n");

  // clean LittleFS before starting tests
  LittleFS.begin(true);
  LittleFS.format();

  storage.setFS(&LittleFS);

  // Configure keys for all supported types
  Serial.println("Configuring keys...");

  // Boolean
  config.configure("bool_key", false);

  // Signed integers
  config.configure("int8_key", static_cast<int8_t>(-42));
  config.configure("int16_key", static_cast<int16_t>(-1000));
  config.configure("int32_key", static_cast<int32_t>(-100000));
#if MYCILA_CONFIG_USE_LONG_LONG
  config.configure("int64_key", static_cast<int64_t>(-9223372036854775807LL));
#endif
  config.configure("int_key", -12345);

  // Unsigned integers
  config.configure("uint8_key", static_cast<uint8_t>(255));
  config.configure("uint16_key", static_cast<uint16_t>(65535));
  config.configure("uint32_key", static_cast<uint32_t>(4294967295UL));
#if MYCILA_CONFIG_USE_LONG_LONG
  config.configure("uint64_key", static_cast<uint64_t>(18446744073709551615ULL));
#endif

  // Floating point
  config.configure("float_key", 3.14159f);
#if MYCILA_CONFIG_USE_DOUBLE
  config.configure("double_key", 2.718281828459045);
#endif

  // String
  config.configure("str_key", "Hello World");
  config.configure("std_str_key", std::string("Hello World"));

  // some custom types
  config.configure("size_t_key", static_cast<size_t>(1));
  config.configure("ulong_key", static_cast<unsigned long>(millis()));

  // Begin config system
  config.begin("Native.ino");

  Serial.println("\n=== Testing Boolean ===");
  assert(config.get<bool>("bool_key") == false);
  assert(config.set<bool>("bool_key", true) == Mycila::config::Status::PERSISTED);
  assert(config.get<bool>("bool_key") == true);
  assert(config.stored("bool_key") == true);
  config.unset("bool_key");
  assert(config.get<bool>("bool_key") == false);
  assert(config.stored("bool_key") == false);
  Serial.println("✓ Boolean tests passed");

  Serial.println("\n=== Testing int8_t ===");
  assert(config.get<int8_t>("int8_key") == -42);
  config.set<int8_t>("int8_key", 127);
  assert(config.get<int8_t>("int8_key") == 127);
  assert(config.stored("int8_key") == true);
  config.unset("int8_key");
  assert(config.get<int8_t>("int8_key") == -42);
  Serial.println("✓ int8_t tests passed");

  Serial.println("\n=== Testing uint8_t ===");
  assert(config.get<uint8_t>("uint8_key") == 255);
  config.set<uint8_t>("uint8_key", 128);
  assert(config.get<uint8_t>("uint8_key") == 128);
  config.unset("uint8_key");
  assert(config.get<uint8_t>("uint8_key") == 255);
  Serial.println("✓ uint8_t tests passed");

  Serial.println("\n=== Testing int16_t ===");
  assert(config.get<int16_t>("int16_key") == -1000);
  config.set<int16_t>("int16_key", 32767);
  assert(config.get<int16_t>("int16_key") == 32767);
  config.unset("int16_key");
  assert(config.get<int16_t>("int16_key") == -1000);
  Serial.println("✓ int16_t tests passed");

  Serial.println("\n=== Testing uint16_t ===");
  assert(config.get<uint16_t>("uint16_key") == 65535);
  config.set<uint16_t>("uint16_key", 32768);
  assert(config.get<uint16_t>("uint16_key") == 32768);
  config.unset("uint16_key");
  assert(config.get<uint16_t>("uint16_key") == 65535);
  Serial.println("✓ uint16_t tests passed");

  Serial.println("\n=== Testing int32_t ===");
  assert(config.get<int32_t>("int32_key") == -100000);
  config.set<int32_t>("int32_key", 2147483647);
  assert(config.get<int32_t>("int32_key") == 2147483647);
  config.unset("int32_key");
  assert(config.get<int32_t>("int32_key") == -100000);
  Serial.println("✓ int32_t tests passed");

  Serial.println("\n=== Testing uint32_t ===");
  assert(config.get<uint32_t>("uint32_key") == 4294967295UL);
  config.set<uint32_t>("uint32_key", 2147483648UL);
  assert(config.get<uint32_t>("uint32_key") == 2147483648UL);
  config.unset("uint32_key");
  assert(config.get<uint32_t>("uint32_key") == 4294967295UL);
  Serial.println("✓ uint32_t tests passed");

#if MYCILA_CONFIG_USE_LONG_LONG
  Serial.println("\n=== Testing int64_t ===");
  assert(config.get<int64_t>("int64_key") == -9223372036854775807LL);
  config.set<int64_t>("int64_key", 9223372036854775807LL);
  assert(config.get<int64_t>("int64_key") == 9223372036854775807LL);
  config.unset("int64_key");
  assert(config.get<int64_t>("int64_key") == -9223372036854775807LL);
  Serial.println("✓ int64_t tests passed");

  Serial.println("\n=== Testing uint64_t ===");
  assert(config.get<uint64_t>("uint64_key") == 18446744073709551615ULL);
  config.set<uint64_t>("uint64_key", 9223372036854775808ULL);
  assert(config.get<uint64_t>("uint64_key") == 9223372036854775808ULL);
  config.unset("uint64_key");
  assert(config.get<uint64_t>("uint64_key") == 18446744073709551615ULL);
  Serial.println("✓ uint64_t tests passed");
#endif

  Serial.println("\n=== Testing int ===");
  assert(config.get<int>("int_key") == -12345);
  config.set<int>("int_key", 54321);
  assert(config.get<int>("int_key") == 54321);
  config.unset("int_key");
  assert(config.get<int>("int_key") == -12345);
  Serial.println("✓ int tests passed");

  Serial.println("\n=== Testing float ===");
  assert(abs(config.get<float>("float_key") - 3.14159f) < 0.00001f);
  config.set<float>("float_key", 2.71828f);
  assert(abs(config.get<float>("float_key") - 2.71828f) < 0.00001f);
  config.unset("float_key");
  assert(abs(config.get<float>("float_key") - 3.14159f) < 0.00001f);
  Serial.println("✓ float tests passed");

#if MYCILA_CONFIG_USE_DOUBLE
  Serial.println("\n=== Testing double ===");
  assert(abs(config.get<double>("double_key") - 2.718281828459045) < 0.000000000000001);
  config.set<double>("double_key", 1.618033988749895);
  assert(abs(config.get<double>("double_key") - 1.618033988749895) < 0.000000000000001);
  config.unset("double_key");
  assert(abs(config.get<double>("double_key") - 2.718281828459045) < 0.000000000000001);
  Serial.println("✓ double tests passed");
#endif

  Serial.println("\n=== Testing String (Str) ===");
  assert(strcmp(config.getString("str_key"), "Hello World") == 0);
  config.setString("str_key", "Goodbye World");
  assert(strcmp(config.getString("str_key"), "Goodbye World") == 0);
  assert(config.stored("str_key") == true);
  config.unset("str_key");
  assert(strcmp(config.getString("str_key"), "Hello World") == 0);
  assert(config.stored("str_key") == false);
  Serial.println("✓ String tests passed");

  // Test type safety - setting wrong type should fail
  // Serial.println("\n=== Testing Type Safety ===");
  // auto result = config.set("bool_key", static_cast<int32_t>(42));
  // assert(result == Mycila::config::Status::ERR_INVALID_TYPE);
  // Serial.println("✓ Type mismatch correctly rejected");

  // Test validators
  Serial.println("\n=== Testing Validators ===");
  config.configure("validated_int", 50, [](const char* key, const Mycila::config::Value& value) {
    return value.as<int>() >= 0 && value.as<int>() <= 100;
  });

  assert(config.set<int>("validated_int", 75) == Mycila::config::Status::PERSISTED);
  assert(config.get<int>("validated_int") == 75);

  assert(config.set<int>("validated_int", 150) == Mycila::config::Status::ERR_INVALID_VALUE);
  assert(config.get<int>("validated_int") == 75); // Should remain unchanged

  assert(config.set<int>("validated_int", -10) == Mycila::config::Status::ERR_INVALID_VALUE);
  assert(config.get<int>("validated_int") == 75);
  Serial.println("✓ Validator tests passed");

  // Test batch operations
  Serial.println("\n=== Testing Batch Operations ===");
  std::map<const char*, Mycila::config::Value> batch = {
    {"bool_key", true},
    {"int32_key", static_cast<int32_t>(999999)},
    {"float_key", 1.41421f},
    {"str_key", Mycila::config::Str("Batch Update")}};

  assert(config.set(batch));
  assert(config.get<bool>("bool_key") == true);
  assert(config.get<int32_t>("int32_key") == 999999);
  assert(abs(config.get<float>("float_key") - 1.41421f) < 0.00001f);
  assert(strcmp(config.getString("str_key"), "Batch Update") == 0);
  Serial.println("✓ Batch operation tests passed");

  // custom type
  Serial.println("\n=== Testing Custom Type ===");
  assert(config.get<unsigned int>("size_t_key") == 1);
  assert(config.get<unsigned long>("ulong_key") < millis());
  Serial.println("✓ Custom type tests passed");

  Serial.println("\n=== Testing get template ===");
  const auto& valueRef = config.get<Mycila::config::Value>("bool_key");
  assert(std::holds_alternative<bool>(valueRef));
  assert(valueRef.as<bool>() == true);
  assert(strcmp(config.get<const char*>("str_key"), "Batch Update") == 0);
  assert(config.get<std::string>("str_key") == "Batch Update");
  Serial.println("✓ get<Value> test passed");

  Serial.println("\n=== Testing as template ===");
  Mycila::config::Value val1 = true;
  assert(val1.as<bool>() == true);
  Mycila::config::Value val2 = static_cast<int32_t>(42);
  assert(val2.as<int32_t>() == 42);
  Mycila::config::Value val3 = 3.14f;
  assert(abs(val3.as<float>() - 3.14f) < 0.00001f);
  Mycila::config::Value val4 = Mycila::config::Str("Test String");
  assert(val4.as<Mycila::config::Str>() == "Test String");
  assert(strcmp(val4.as<const char*>(), "Test String") == 0);
  assert(val4.as<std::string>() == "Test String");
  Serial.println("✓ as<T>() tests passed");

  Serial.println("\n=== Testing Value.toString() ===");
  assert(Mycila::config::Value(true).toString() == MYCILA_CONFIG_VALUE_TRUE);
  assert(Mycila::config::Value(false).toString() == MYCILA_CONFIG_VALUE_FALSE);
  assert(Mycila::config::Value(static_cast<int8_t>(-42)).toString() == "-42");
  assert(Mycila::config::Value(static_cast<uint8_t>(255)).toString() == "255");
  assert(Mycila::config::Value(static_cast<int32_t>(12345)).toString() == "12345");
  assert(Mycila::config::Value(static_cast<uint32_t>(67890)).toString() == "67890");
  assert(Mycila::config::Value(3.14159f).toString().substr(0, 4) == "3.14");
  assert(Mycila::config::Value(Mycila::config::Str("Hello")).toString() == "Hello");
  Serial.println("✓ toString() tests passed");

  Serial.println("\n=== Testing Value::fromString() ===");
  // Boolean parsing
  auto boolVal1 = Mycila::config::Value::fromString(MYCILA_CONFIG_VALUE_TRUE, Mycila::config::Value(false));
  assert(boolVal1.has_value() && boolVal1.value().as<bool>() == true);

  auto boolVal2 = Mycila::config::Value::fromString(MYCILA_CONFIG_VALUE_FALSE, Mycila::config::Value(true));
  assert(boolVal2.has_value() && boolVal2.value().as<bool>() == false);

  // Integer parsing
  auto intVal = Mycila::config::Value::fromString("12345", Mycila::config::Value(static_cast<int32_t>(0)));
  assert(intVal.has_value() && intVal.value().as<int32_t>() == 12345);
  auto int8Val = Mycila::config::Value::fromString("-42", Mycila::config::Value(static_cast<int8_t>(0)));
  assert(int8Val.has_value() && int8Val.value().as<int8_t>() == -42);

  auto uint8Val = Mycila::config::Value::fromString("255", Mycila::config::Value(static_cast<uint8_t>(0)));
  assert(uint8Val.has_value() && uint8Val.value().as<uint8_t>() == 255);

  auto uint32Val = Mycila::config::Value::fromString("4294967295", Mycila::config::Value(static_cast<uint32_t>(0)));
  assert(uint32Val.has_value() && uint32Val.value().as<uint32_t>() == 4294967295UL);

  // Float parsing
  auto floatVal = Mycila::config::Value::fromString("3.14", Mycila::config::Value(0.0f));
  assert(floatVal.has_value() && abs(floatVal.value().as<float>() - 3.14f) < 0.01f);

#if MYCILA_CONFIG_USE_DOUBLE
  // Double parsing
  auto doubleVal = Mycila::config::Value::fromString("2.718281828", Mycila::config::Value(0.0));
  assert(doubleVal.has_value() && abs(doubleVal.value().as<double>() - 2.718281828) < 0.000000001);
#endif

  // String parsing
  auto strVal = Mycila::config::Value::fromString("Hello World", Mycila::config::Value(Mycila::config::Str("")));
  assert(strVal.has_value() && strcmp(strVal.value().as<Mycila::config::Str>().c_str(), "Hello World") == 0);

  // Invalid parsing
  auto invalidInt = Mycila::config::Value::fromString("not_a_number", Mycila::config::Value(static_cast<int32_t>(999)));
  assert(!invalidInt.has_value());

  auto invalidFloat = Mycila::config::Value::fromString("abc", Mycila::config::Value(1.23f));
  assert(!invalidFloat.has_value());

  Serial.println("✓ fromString() tests passed");

  Serial.println("\n=== Testing Round-trip Conversion ===");
  // Test that toString() and fromString() are inverses
  Mycila::config::Value original1(true);
  auto str1 = original1.toString();
  auto parsed1 = Mycila::config::Value::fromString(str1.c_str(), Mycila::config::Value(false));
  assert(parsed1.has_value() && parsed1.value().as<bool>() == true);

  Mycila::config::Value original2(static_cast<int32_t>(42));
  auto str2 = original2.toString();
  auto parsed2 = Mycila::config::Value::fromString(str2.c_str(), Mycila::config::Value(static_cast<int32_t>(0)));
  assert(parsed2.has_value() && parsed2.value().as<int32_t>() == 42);

  Mycila::config::Value original3(Mycila::config::Str("Test"));
  auto str3 = original3.toString();
  auto parsed3 = Mycila::config::Value::fromString(str3.c_str(), Mycila::config::Value(Mycila::config::Str("")));
  assert(parsed3.has_value() && strcmp(parsed3.value().as<Mycila::config::Str>().c_str(), "Test") == 0);

  Serial.println("✓ Round-trip conversion tests passed");

  Serial.println("\n=== All Tests Passed! ===");
  Serial.printf("\nHeap usage: %zu bytes\n", config.heapUsage());
  Serial.printf("Free heap: %" PRIu32 " bytes\n", ESP.getFreeHeap());
}

void loop() {
  vTaskDelete(NULL);
}
