// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#include <MycilaConfig.h>
#include <MycilaConfigStorageNVS.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID    "wifi_ssid"
#define KEY_WIFI_PWD     "wifi_pwd"

Mycila::config::NVS storage;
Mycila::config::Config config(storage);

uint8_t getLogLevel() { return config.get<bool>(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  // just to clear storage before the test
  storage.begin("Json.ino");
  storage.removeAll();
  storage.end();

  config.configure(KEY_DEBUG_ENABLE, false);
  config.configure("a int", 3);
  config.configure("a float", 3.3f);
  config.configure(KEY_WIFI_SSID);
  config.configure(KEY_WIFI_PWD);

  config.begin("Json.ino");

  JsonDocument doc;
  config.toJson(doc.to<JsonObject>());
  serializeJson(doc, Serial);
  Serial.println();

  config.backup(Serial);

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_INFO);

  config.set<bool>(KEY_DEBUG_ENABLE, true);

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_DEBUG);
  doc.clear();
  config.toJson(doc.to<JsonObject>());
  serializeJson(doc, Serial);
  Serial.println();
}

void loop() {
  vTaskDelete(NULL);
}
