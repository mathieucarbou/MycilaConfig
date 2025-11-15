// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <MycilaConfig.h>
#include <MycilaNVSStorage.h>
#include <Preferences.h>

Mycila::NVSStorage storage;
Mycila::Config config(storage);
Preferences prefs;

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
  prefs.begin("CONFIG", false);
  prefs.clear();
  prefs.putString("key4", "bar");
  prefs.end();
  prefs.begin("CONFIG", true);

  // listeners

  config.listen([](const char* key, const char* newValue) {
    Serial.printf("(listen) '%s' => '%s'\n", key, newValue);
  });

  config.listen([]() {
    Serial.println("(restored)");
  });

  // begin()

  config.begin();

  // configure()

  config.configure("key1", MYCILA_CONFIG_VALUE_FALSE);
  config.configure("key2", "");
  config.configure("key3");
  config.configure("key4", "foo");
  config.configure("key5", "baz");
  config.configure("key6", std::to_string(6));
  config.configure("key7", "-1");
  config.configure("key10");

  // tests

  assertEquals(config.get("key1"), MYCILA_CONFIG_VALUE_FALSE);
  assertEquals(config.get("key2"), "");
  assertEquals(config.get("key3"), "");

  // check exists key
  assert(config.configured("key4"));

  // set global validator
  assert(config.setValidator([](const char* key, const char* newValue) {
    Serial.printf("(global validator) '%s' => '%s'\n", key, newValue);
    return true;
  }));

  // set key
  assert(config.set("key1", MYCILA_CONFIG_VALUE_TRUE));
  assertEquals(config.get("key1"), MYCILA_CONFIG_VALUE_TRUE);
  assert(prefs.isKey("key1"));

  // set key to same value => no change
  assert(config.set("key1", MYCILA_CONFIG_VALUE_TRUE) == Mycila::Config::Status::PERSISTED);
  assert(config.set("key1", MYCILA_CONFIG_VALUE_TRUE));

  // cache stored key
  assertEquals(config.get("key4"), "bar"); // load key and cache

  // set key to same value => no change
  assert(config.set("key4", "bar") == Mycila::Config::Status::PERSISTED);
  assert(config.set("key4", "bar"));

  // set stored key to default value
  assert(config.set("key4", "foo"));
  assertEquals(config.get("key4"), "foo");
  assert(prefs.isKey("key4"));

  // set stored key to other value
  assert(config.set("key4", "bar"));
  assertEquals(config.get("key4"), "bar");

  // unset global validator
  assert(config.setValidator(nullptr));

  // unset stored key
  assert(config.unset("key4"));
  assert(!prefs.isKey("key4"));
  assertEquals(config.get("key4"), "foo");

  // unset non-existing key => noop
  assert(config.unset("key4") == Mycila::Config::Status::REMOVED);
  assert(config.unset("key4"));
  assertEquals(config.get("key4"), "foo");

  // set validator
  assert(config.setValidator("key4", [](const char* key, const char* newValue) {
    Serial.printf("(validator) '%s' => '%s'\n", key, newValue);
    return strcmp(newValue, "baz") == 0;
  }));

  // try set a permitted value
  assert(config.set("key4", "baz"));
  assertEquals(config.get("key4"), "baz");

  // try set a NOT permitted value
  assert(config.set("key4", "bar") == Mycila::Config::Status::ERR_INVALID_VALUE);
  assert(!config.set("key4", "bar"));
  assertEquals(config.get("key4"), "baz");

  // unset validator
  assert(config.setValidator("key4", nullptr));

  // set un-stored to default value => no change
  assert(config.set("key5", "baz") == Mycila::Config::Status::DEFAULTED);
  assert(config.set("key5", "baz"));

  // remove an inexisting key
  assert(config.unset("key5") == Mycila::Config::Status::REMOVED);
  assert(config.unset("key5"));

  // try set to empty value a key which has a default value
  assertEquals(config.get("key5"), "baz");
  assert(config.set("key5", ""));
  assertEquals(config.get("key5"), "");

  config.backup(Serial);

  config.set("key1", "value1");
  config.set("key2", "value2");
  config.unset("key4"); // back to default value

  config.unset("key7"); // back to default value, but key was not stored
  assert(config.getInt("key7") == -1);

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
  assert(config.get("key11") == nullptr);

  assertEquals(config.get("key1"), "");
  assertEquals(config.get("key2"), "");
  assertEquals(config.get("key3"), "value3");
  assertEquals(config.get("key4"), "foo"); // default value

  assertEquals(config.get("key6"), "6");
  config.set("key6", std::to_string(7)); // deleter should be called to delete buffer
  assertEquals(config.get("key6"), "7");

  Serial.printf("Free heap: %" PRIu32 " bytes\n", ESP.getFreeHeap());
  for (size_t i = 0; i < 100; i++) {
    config.set("key10", "some long string to eat memory: " + std::to_string(i)); // key saved and cache erased => buffer allocation should be freed
    const char* v = config.get("key10");                                         // key cached
    Serial.printf("key10 = %s\n", v);
  }
  config.unset("key10"); // cache erased
  Serial.printf("Free heap: %" PRIu32 " bytes\n", ESP.getFreeHeap());
}

void loop() {
  vTaskDelete(NULL);
}
