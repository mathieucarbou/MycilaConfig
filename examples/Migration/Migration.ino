// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#include <Esp.h>
#include <HardwareSerial.h>
#include <assert.h>

#include <MycilaConfig.h>
#include <MycilaConfigStorageNVS.h>

Mycila::config::NVS storage;
Mycila::config::Config config(storage);
Mycila::config::Migration migration(config);

static bool secondPass = false;

static void migrate() {
  assert(migration.begin("Migration.ino"));

  // specific type to type migration (non-string)
  assert(migration.migrate<Mycila::config::Str>("key6", [](const Mycila::config::Str& value) -> std::optional<Mycila::config::Value> {
    const char* v = value.c_str();
    if (strncmp(v, "50", 2) == 0)
      return Mycila::config::Value(static_cast<uint8_t>(50));
    if (strncmp(v, "60", 2) == 0)
      return Mycila::config::Value(static_cast<uint8_t>(60));
    return std::nullopt; // migration failed
  }) == (secondPass ? Mycila::config::Status::ERR_UNKNOWN_KEY : Mycila::config::Status::PERSISTED));

  assert(migration.migrate<uint8_t>("key7", [](const uint8_t& value) -> std::optional<Mycila::config::Value> {
    return Mycila::config::Value(static_cast<uint16_t>(value));
  }) == (secondPass ? Mycila::config::Status::ERR_UNKNOWN_KEY : Mycila::config::Status::PERSISTED));

  assert(migration.migrate<uint16_t>("key8", [](const uint16_t& value) -> std::optional<Mycila::config::Value> {
    return Mycila::config::Value(static_cast<uint8_t>(value));
  }) == (secondPass ? Mycila::config::Status::ERR_UNKNOWN_KEY : Mycila::config::Status::PERSISTED));

  assert(migration.migrate<uint16_t>("key9", [](const uint16_t& value) -> std::optional<Mycila::config::Value> {
    return Mycila::config::Value(static_cast<uint8_t>(value <= UINT8_MAX ? value : UINT8_MAX)); // we clip to max
  }) == (secondPass ? Mycila::config::Status::ERR_UNKNOWN_KEY : Mycila::config::Status::PERSISTED));

  assert(migration.migrate<uint16_t>("key10", [](const uint16_t& value) -> std::optional<Mycila::config::Value> {
    if (value <= UINT8_MAX)
      return Mycila::config::Value(static_cast<uint8_t>(value));
    return std::nullopt;
  }) == (secondPass ? Mycila::config::Status::ERR_UNKNOWN_KEY : Mycila::config::Status::REMOVED));

  assert(migration.migrateFromString());

  migration.end();
}

static void test() {
  assert(config.get<bool>("key1") == true);
  assert(config.get<int32_t>("key2") == 12345);
  assert(fabs(config.get<float>("key3") - 3.14159f) < 0.0001f);
  assert(config.get<std::string>("key4") == "already a string");
  assert(config.get<int32_t>("key5") == 10);
  assert(config.get<uint8_t>("key6") == 60);
  assert(config.get<uint16_t>("key7") == 1);
  assert(config.get<uint8_t>("key8") == 1);
  assert(config.get<uint8_t>("key9") == 255); // clipped to max
  assert(!config.storage().hasKey("key10"));  // removed because migration failed
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial.println("\n=== Migration Test Example ===\n");

  // prepare storage for migration test
  assert(storage.begin("Migration.ino"));
  storage.removeAll();
  storage.storeString("key1", "true");             // we want to migrate to bool
  storage.storeString("key2", "12345");            // we want to migrate to int32_t
  storage.storeString("key3", "3.14159");          // we want to migrate to float
  storage.storeString("key4", "already a string"); // already correct type
  storage.storeI32("key5", 10);                    // already correct type
  storage.storeString("key6", "60 Hz");            // we want to migrate to uint8_t but the stored value is not a pure number
  storage.storeU8("key7", 1);                      // we want to migrate to uint16_t
  storage.storeU16("key8", 1);                     // we want to migrate to uint8_t
  storage.storeU16("key9", 256);                   // we want to migrate to uint8_t but we want to clip to max
  storage.storeU16("key10", 12345);                // we want to remove this key if value is invalid, otherwise store the uint8_t value
  storage.end();

  // configure keys with the new types
  config.configure("key1", false);
  config.configure("key2", static_cast<int32_t>(0));
  config.configure("key3", 0.0f);
  config.configure("key4", "default string");
  config.configure("key5", static_cast<int32_t>(0));
  config.configure("key6", static_cast<uint8_t>(0));
  config.configure("key7", static_cast<uint16_t>(0));
  config.configure("key8", static_cast<uint8_t>(0));
  config.configure("key9", static_cast<uint8_t>(0));
  config.configure("key10", static_cast<uint8_t>(0));

  config.listen([](const char* key, const Mycila::config::Value& newValue) {
    assert(false); // listener must not be called during migration
  });

  // call migration
  migrate();
  // begin config system (will trigger migration)
  assert(config.begin("Migration.ino"));
  // test migrated values
  test();
  // end config
  config.end();

  // second round with migrated values now:
  secondPass = true;
  migrate();
  assert(config.begin("Migration.ino"));
  test();
  config.end();

  Serial.println("Migration test completed successfully!");
}

void loop() {
  vTaskDelete(NULL);
}
