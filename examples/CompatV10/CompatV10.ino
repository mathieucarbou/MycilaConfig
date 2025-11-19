// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <Esp.h>
#include <HardwareSerial.h>

#include <MycilaConfig.h>
#include <MycilaConfigStorageNVS.h>
#include <MycilaConfigV10.h>

Mycila::config::NVS storage;
Mycila::config::Config configNew(storage);
Mycila::config::ConfigV10 config(configNew);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial.println("\n=== ConfigV10 Compatibility Wrapper Example ===\n");
  Serial.println("This example demonstrates using the v10 string-based API");
  Serial.println("with the ConfigV10 compatibility wrapper for v11+\n");
  Serial.println("Note: Validators and callbacks use the new v11 API (not backward compatible)\n");

  // Clear storage for clean test
  storage.begin("CompatV10");
  storage.removeAll();
  storage.end();

  // Configure keys using v10 string-based API
  Serial.println("=== Configuring keys (v10 API) ===");
  config.configure("wifi_ssid", "MyNetwork");
  config.configure("wifi_pwd", "");
  config.configure("port", "8080");
  config.configure("timeout", "30");
  config.configure("debug_enable", "false");
  config.configure("threshold", "25.5");

  // Begin config system
  config.begin("CompatV10");

  // Test v10 string-based getters
  Serial.println("\n=== Testing v10 getString() ===");
  Serial.printf("wifi_ssid = '%s'\n", config.getString("wifi_ssid"));
  Serial.printf("port = '%s'\n", config.getString("port"));
  Serial.printf("debug_enable = '%s'\n", config.getString("debug_enable"));

  // Test v10 typed getters
  Serial.println("\n=== Testing v10 typed getters ===");
  Serial.printf("getBool('debug_enable') = %s\n", config.getBool("debug_enable") ? "true" : "false");
  Serial.printf("getInt('port') = %d\n", config.getInt("port"));
  Serial.printf("getLong('timeout') = %ld\n", config.getLong("timeout"));
  Serial.printf("getFloat('threshold') = %.1f\n", config.getFloat("threshold"));

  // Test v10 string-based setters
  Serial.println("\n=== Testing v10 setString() ===");
  config.setString("wifi_ssid", "NewNetwork");
  Serial.printf("After setString: wifi_ssid = '%s'\n", config.getString("wifi_ssid"));

  // Test v10 typed setters
  Serial.println("\n=== Testing v10 typed setters ===");
  config.setBool("debug_enable", true);
  Serial.printf("After setBool: debug_enable = %s\n", config.getBool("debug_enable") ? "true" : "false");

  config.setInt("port", 9000);
  Serial.printf("After setInt: port = %d\n", config.getInt("port"));

  config.setFloat("threshold", 42.3f);
  Serial.printf("After setFloat: threshold = %.1f\n", config.getFloat("threshold"));

  // Test stored() and configured()
  Serial.println("\n=== Testing stored() and configured() ===");
  Serial.printf("configured('port'): %s\n", config.configured("port") ? "true" : "false");
  Serial.printf("stored('port'): %s\n", config.stored("port") ? "true" : "false");
  Serial.printf("configured('unknown'): %s\n", config.configured("unknown") ? "true" : "false");

  // Test unset
  Serial.println("\n=== Testing unset() ===");
  Serial.printf("Before unset - wifi_ssid: '%s'\n", config.getString("wifi_ssid"));
  config.unset("wifi_ssid");
  Serial.printf("After unset - wifi_ssid: '%s'\n", config.getString("wifi_ssid"));
  Serial.printf("stored('wifi_ssid'): %s\n", config.stored("wifi_ssid") ? "true" : "false");

  // Test isEmpty() and isEqual()
  Serial.println("\n=== Testing isEmpty() and isEqual() ===");
  Serial.printf("isEmpty('wifi_pwd'): %s\n", config.isEmpty("wifi_pwd") ? "true" : "false");
  Serial.printf("isEqual('wifi_ssid', 'MyNetwork'): %s\n", config.isEqual("wifi_ssid", "MyNetwork") ? "true" : "false");

  // Test isPasswordKey() and isEnableKey()
  Serial.println("\n=== Testing key helpers ===");
  Serial.printf("isPasswordKey('wifi_pwd'): %s\n", config.isPasswordKey("wifi_pwd") ? "true" : "false");
  Serial.printf("isEnableKey('debug_enable'): %s\n", config.isEnableKey("debug_enable") ? "true" : "false");

  // Test backup
  Serial.println("\n=== Testing backup() ===");
  Serial.println("Current configuration:");
  config.backup(Serial, false);

  // Test batch set
  Serial.println("\n=== Testing batch set ===");
  std::map<const char*, std::string> batch = {
    {"wifi_ssid", "BatchNetwork"},
    {"port", "3000"},
    {"debug_enable", "false"}
  };
  config.set(batch);
  Serial.println("After batch set:");
  config.backup(Serial, false);

  // Test restore
  Serial.println("\n=== Testing restore() ===");
  const char* backupData = 
    "wifi_ssid=RestoredNetwork\n"
    "port=5000\n"
    "debug_enable=true\n";
  
  config.restore(backupData);
  Serial.println("After restore:");
  Serial.printf("wifi_ssid = '%s'\n", config.getString("wifi_ssid"));
  Serial.printf("port = %d\n", config.getInt("port"));
  Serial.printf("debug_enable = %s\n", config.getBool("debug_enable") ? "true" : "false");

  Serial.println("\n=== All v10 compatibility tests completed ===");
  Serial.printf("\nHeap usage: %zu bytes\n", config.heapUsage());
  Serial.printf("Free heap: %" PRIu32 " bytes\n", ESP.getFreeHeap());
}

void loop() {
  vTaskDelete(NULL);
}
