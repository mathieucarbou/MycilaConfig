// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <Esp.h>
#include <HardwareSerial.h>

#include <MycilaConfig.h>
#include <MycilaConfigStorageNVS.h>

Mycila::config::NVS storage;
Mycila::config::Config config(storage);

static void assertEquals(const char* actual, const char* expected) {
  if (strcmp(actual, expected) != 0) {
    Serial.printf("Expected '%s' but got '%s'\n", expected, actual);
    assert(false);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  // prepare storage for tests
  storage.begin("Test.ino");
  storage.removeAll();
  storage.storeString("key4", "bar");
  storage.end();

  // listeners

  config.listen([](const char* key, const Mycila::config::Value& newValue) {
    Serial.printf("(listen) '%s' => '%s'\n", key, newValue.as<const char*>());
  });

  config.listen([]() {
    Serial.println("(restored)");
  });

  // configure()

  config.configure("key1", MYCILA_CONFIG_VALUE_FALSE);
  config.configure("key2", "");
  config.configure("key3");
  config.configure("key4", "foo");
  config.configure("key5", "baz");
  config.configure("key6", std::to_string(6).c_str());
  config.configure("key10");

  // begin()

  config.begin("Test.ino", true);

  // tests

  assertEquals(config.getString("key1"), MYCILA_CONFIG_VALUE_FALSE);
  assertEquals(config.getString("key2"), "");
  assertEquals(config.getString("key3"), "");

  // check exists key
  assert(config.configured("key4"));

  // set global validator
  assert(config.setValidator([](const char* key, const Mycila::config::Value& newValue) {
    Serial.printf("(global validator) '%s' => '%s'\n", key, newValue.as<const char*>());
    return true;
  }));

  // set key
  assert(config.setString("key1", MYCILA_CONFIG_VALUE_TRUE));
  assertEquals(config.getString("key1"), MYCILA_CONFIG_VALUE_TRUE);
  assert(storage.hasKey("key1"));

  // set key to same value => no change
  assert(config.setString("key1", MYCILA_CONFIG_VALUE_TRUE) == Mycila::config::Status::PERSISTED);
  assert(config.setString("key1", MYCILA_CONFIG_VALUE_TRUE));

  // cache stored key
  assertEquals(config.getString("key4"), "bar"); // load key and cache

  // set key to same value => no change
  assert(config.setString("key4", "bar") == Mycila::config::Status::PERSISTED);
  assert(config.setString("key4", "bar"));

  // set stored key to default value
  assert(config.setString("key4", "foo"));
  assertEquals(config.getString("key4"), "foo");
  assert(storage.hasKey("key4"));

  // set stored key to other value
  assert(config.setString("key4", "bar"));
  assertEquals(config.getString("key4"), "bar");

  // unset global validator
  assert(config.setValidator(nullptr));

  // unset stored key
  assert(config.unset("key4"));
  assert(!storage.hasKey("key4"));
  assertEquals(config.getString("key4"), "foo");

  // unset non-existing key => noop
  assert(config.unset("key4") == Mycila::config::Status::REMOVED);
  assert(config.unset("key4"));
  assertEquals(config.getString("key4"), "foo");

  // set validator
  assert(config.setValidator("key4", [](const char* key, const Mycila::config::Value& newValue) {
    Serial.printf("(validator) '%s' => '%s'\n", key, newValue.as<const char*>());
    return std::holds_alternative<Mycila::config::Str>(newValue) && newValue.as<Mycila::config::Str>() == "baz";
  }));

  // try set a permitted value
  assert(config.setString("key4", "baz"));
  assertEquals(config.getString("key4"), "baz");

  // try set a NOT permitted value
  assert(config.setString("key4", "bar") == Mycila::config::Status::ERR_INVALID_VALUE);
  assert(!config.setString("key4", "bar"));
  assertEquals(config.getString("key4"), "baz");

  // unset validator
  assert(config.setValidator("key4", nullptr));

  // set un-stored to default value => no change
  assert(config.setString("key5", "baz") == Mycila::config::Status::DEFAULTED);
  assert(config.setString("key5", "baz"));

  // remove an inexisting key
  assert(config.unset("key5") == Mycila::config::Status::REMOVED);
  assert(config.unset("key5"));

  // try set to empty value a key which has a default value
  assertEquals(config.getString("key5"), "baz");
  assert(config.setString("key5", ""));
  assertEquals(config.getString("key5"), "");

  config.backup(Serial);

  config.setString("key1", "value1");
  config.setString("key2", "value2");
  config.unset("key4"); // back to default value

  // Should export everything
  // key1=value1
  // key2=value2
  // key3=
  // key4=foo
  // key5=
  // key6=6
  config.backup(Serial);

  // Should export ONLY values set by user
  // key1=value1
  // key2=value2
  // key5=
  config.backup(Serial, false);

  // Should only persist if required
  // set(key1, ): PERSISTED
  // set(key2, ): PERSISTED
  // set(key3, value3): PERSISTED
  // set(key4, foo): DEFAULTED
  config.restore("key1=\nkey2=\nkey3=value3\nkey4=foo\n");

  // key not configured:
  // config.getString("key11"); // crashes

  assertEquals(config.getString("key1"), "");
  assertEquals(config.getString("key2"), "");
  assertEquals(config.getString("key3"), "value3");
  assertEquals(config.getString("key4"), "foo"); // default value

  assertEquals(config.getString("key6"), "6");
  config.setString("key6", std::to_string(7).c_str()); // deleter should be called to delete buffer
  assertEquals(config.getString("key6"), "7");

  Serial.printf("Free heap: %" PRIu32 " bytes\n", ESP.getFreeHeap());
  for (size_t i = 0; i < 100; i++) {
    config.setString("key10", ("some long string to eat memory: " + std::to_string(i)).c_str()); // key saved and cache erased => buffer allocation should be freed
    const char* v = config.getString("key10");                                                   // key cached
    Serial.printf("key10 = %s\n", v);
  }
  config.unset("key10"); // cache erased
  Serial.printf("Free heap: %" PRIu32 " bytes\n", ESP.getFreeHeap());
}

void loop() {
  vTaskDelete(NULL);
}
