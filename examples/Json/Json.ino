// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <ArduinoJson.h>
#include <StreamString.h>

#include <MycilaConfig.h>
#include <MycilaConfigStorageNVS.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PWD "wifi_pwd"

Mycila::ConfigStorageNVS storage;
Mycila::Config config(storage);

uint8_t getLogLevel() { return config.get<bool>(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  config.configure(KEY_DEBUG_ENABLE, MYCILA_CONFIG_VALUE_FALSE);
  config.configure(KEY_WIFI_SSID);
  config.configure(KEY_WIFI_PWD);

  config.begin("Json.ino");
}

void loop() {
  JsonDocument doc;
  config.toJson(doc.to<JsonObject>());
  serializeJson(doc, Serial);
  Serial.println();

  StreamString content;
  content.reserve(1024);
  config.backup(content);
  Serial.println(content);

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_INFO);

  config.set<bool>(KEY_DEBUG_ENABLE, !config.get<bool>(KEY_DEBUG_ENABLE));

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_DEBUG);

  delay(5000);
}
