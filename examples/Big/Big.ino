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

unsigned long lastHeapLog = 0;
unsigned long operationCount = 0;

void printHeap(const char* label) {
  Serial.printf("[HEAP] %s - Free: %" PRIu32 " bytes, Min: %" PRIu32 " bytes, Config: %zu bytes\n",
                label,
                ESP.getFreeHeap(),
                ESP.getMinFreeHeap(),
                config.heapUsage());
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial.println("\n=== BigConfig Example - 150 Keys ===\n");

  // Configure 150 keys with various default values using native types
  config.configure("wifi_ssid", "MyNetwork");
  config.configure("wifi_pwd", "secret123");
  config.configure("wifi_enable", true);
  config.configure("mqtt_host", "192.168.1.100");
  config.configure("mqtt_port", 1883);
  config.configure("mqtt_user", "admin");
  config.configure("mqtt_pwd", "mqtt_pass");
  config.configure("mqtt_enable", false);
  config.configure("ntp_server", "pool.ntp.org");
  config.configure("ntp_tz", "UTC");
  config.configure("ntp_enable", true);
  config.configure("web_port", 80);
  config.configure("web_user", "admin");
  config.configure("web_pwd", "admin");
  config.configure("web_enable", true);
  config.configure("ap_ssid", "ESP-Config");
  config.configure("ap_pwd", "12345678");
  config.configure("ap_enable", false);
  config.configure("hostname", "esp32-device");
  config.configure("device_name", "My ESP32");
  config.configure("device_id", "esp32-001");
  config.configure("relay1_name", "Relay 1");
  config.configure("relay1_enable", true);
  config.configure("relay2_name", "Relay 2");
  config.configure("relay2_enable", false);
  config.configure("relay3_name", "Relay 3");
  config.configure("relay3_enable", false);
  config.configure("relay4_name", "Relay 4");
  config.configure("relay4_enable", true);
  config.configure("sensor1_name", "Temperature");
  config.configure("sensor1_type", "DHT22");
  config.configure("sensor1_pin", static_cast<uint8_t>(4));
  config.configure("sensor1_enbl", true);
  config.configure("sensor2_name", "Humidity");
  config.configure("sensor2_type", "DHT22");
  config.configure("sensor2_pin", static_cast<uint8_t>(5));
  config.configure("sensor2_enbl", true);
  config.configure("sensor3_name", "Pressure");
  config.configure("sensor3_type", "BMP280");
  config.configure("sensor3_pin", static_cast<uint8_t>(21));
  config.configure("sensor3_enbl", false);
  config.configure("led1_pin", static_cast<uint8_t>(2));
  config.configure("led1_enable", true);
  config.configure("led2_pin", static_cast<uint8_t>(13));
  config.configure("led2_enable", false);
  config.configure("led3_pin", static_cast<uint8_t>(14));
  config.configure("led3_enable", false);
  config.configure("led4_pin", static_cast<uint8_t>(15));
  config.configure("led4_enable", true);
  config.configure("btn1_pin", static_cast<uint8_t>(18));
  config.configure("btn1_enable", true);
  config.configure("btn2_pin", static_cast<uint8_t>(19));
  config.configure("btn2_enable", false);
  config.configure("log_level", "DEBUG");
  config.configure("log_enable", true);
  config.configure("telnet_port", 23);
  config.configure("telnet_enable", false);
  config.configure("serial_baud", 115200);
  config.configure("serial_enable", true);
  config.configure("i2c_sda", static_cast<uint8_t>(21));
  config.configure("i2c_scl", static_cast<uint8_t>(22));
  config.configure("i2c_freq", 100000);
  config.configure("i2c_enable", true);
  config.configure("spi_mosi", static_cast<uint8_t>(23));
  config.configure("spi_miso", static_cast<uint8_t>(19));
  config.configure("spi_sclk", static_cast<uint8_t>(18));
  config.configure("spi_cs", static_cast<uint8_t>(5));
  config.configure("spi_enable", false);
  config.configure("pwm1_pin", static_cast<uint8_t>(25));
  config.configure("pwm1_freq", 5000);
  config.configure("pwm1_duty", static_cast<uint8_t>(128));
  config.configure("pwm1_enable", true);
  config.configure("pwm2_pin", static_cast<uint8_t>(26));
  config.configure("pwm2_freq", 5000);
  config.configure("pwm2_duty", static_cast<uint8_t>(64));
  config.configure("pwm2_enable", false);
  config.configure("adc1_pin", static_cast<uint8_t>(34));
  config.configure("adc1_enable", true);
  config.configure("adc2_pin", static_cast<uint8_t>(35));
  config.configure("adc2_enable", false);
  config.configure("dac1_pin", static_cast<uint8_t>(25));
  config.configure("dac1_enable", false);
  config.configure("dac2_pin", static_cast<uint8_t>(26));
  config.configure("dac2_enable", false);
  config.configure("timer1_period", 1000);
  config.configure("timer1_enable", true);
  config.configure("timer2_period", 5000);
  config.configure("timer2_enable", false);
  config.configure("watchdog_time", 30000);
  config.configure("watchdog_enbl", true);
  config.configure("ota_port", 3232);
  config.configure("ota_pwd", "ota_pass");
  config.configure("ota_enable", true);
  config.configure("mdns_name", "esp32");
  config.configure("mdns_enable", true);
  config.configure("sntp_server1", "time.google.com");
  config.configure("sntp_server2", "time.nist.gov");
  config.configure("sntp_enable", true);
  config.configure("temp_unit", "C");
  config.configure("temp_offset", 0.0f);
  config.configure("pres_unit", "hPa");
  config.configure("pres_offset", 0.0f);
  config.configure("hum_offset", 0.0f);
  config.configure("altitude", 100);
  config.configure("latitude", 45.5017f);
  config.configure("longitude", -73.5673f);
  config.configure("timezone_off", -5);
  config.configure("dst_enable", true);
  config.configure("disp_bright", static_cast<uint8_t>(128));
  config.configure("disp_enable", true);
  config.configure("disp_timeout", 30000);
  config.configure("disp_type", "SSD1306");
  config.configure("disp_width", static_cast<uint8_t>(128));
  config.configure("disp_height", static_cast<uint8_t>(64));
  config.configure("disp_addr", "0x3C");
  config.configure("rtc_type", "DS3231");
  config.configure("rtc_enable", false);
  config.configure("sd_cs_pin", static_cast<uint8_t>(5));
  config.configure("sd_enable", false);
  config.configure("sd_format", false);
  config.configure("bat_adc_pin", static_cast<uint8_t>(36));
  config.configure("bat_v_divider", 2.0f);
  config.configure("bat_enable", true);
  config.configure("bat_low_volt", 3.3f);
  config.configure("bat_high_volt", 4.2f);
  config.configure("sleep_mode", "light");
  config.configure("sleep_time", 60000);
  config.configure("sleep_enable", false);
  config.configure("alarm1_hour", static_cast<uint8_t>(7));
  config.configure("alarm1_min", static_cast<uint8_t>(30));
  config.configure("alarm1_enable", true);
  config.configure("alarm2_hour", static_cast<uint8_t>(19));
  config.configure("alarm2_min", static_cast<uint8_t>(0));
  config.configure("alarm2_enable", false);
  config.configure("rgb_pin", static_cast<uint8_t>(27));
  config.configure("rgb_count", static_cast<uint8_t>(8));
  config.configure("rgb_bright", static_cast<uint8_t>(50));
  config.configure("rgb_enable", true);
  config.configure("ir_rx_pin", static_cast<uint8_t>(16));
  config.configure("ir_tx_pin", static_cast<uint8_t>(17));
  config.configure("ir_enable", false);
  config.configure("rf_rx_pin", static_cast<uint8_t>(32));
  config.configure("rf_tx_pin", static_cast<uint8_t>(33));
  config.configure("rf_enable", false);
  config.configure("auth_user1", "admin");
  config.configure("auth_pwd1", "admin123");
  config.configure("auth_user2", "user");
  config.configure("auth_pwd2", "user123");

  printHeap("After configure()");

  printHeap("Before begin()");

  config.begin("Big", true);

  printHeap("After begin()");

  // Register a change listener
  config.listen([](const char* key, const std::optional<Mycila::config::Value>& newValue) {
    if (newValue.has_value()) {
      Serial.printf("[CHANGE] %s = %s\n", key, Mycila::config::toString(newValue.value()).c_str());
    } else {
      Serial.printf("[UNSET] %s\n", key);
    }
  });

  Serial.println("\n=== Configuration Complete ===");
  Serial.printf("Total keys configured: %d\n", config.keys().size());
  Serial.println("Starting random operations...\n");

  lastHeapLog = millis();
}

void loop() {
  // Log heap every 2 seconds
  if (millis() - lastHeapLog >= 2000) {
    printHeap("Loop");
    Serial.printf("Operations completed: %lu\n\n", operationCount);
    lastHeapLog = millis();
  }

  // Perform random operations
  int op = random(0, 100);

  if (op < 40) {
    // 40% chance: GET operation
    const auto& keys = config.keys();
    const char* key = keys[random(0, keys.size())].name;

    // Get value using typed API based on key type
    if (config.configured(key)) {
      auto value = config.get<Mycila::config::Value>(key);
      Serial.printf("[GET] %s = %s\n", key, Mycila::config::toString(value).c_str());
    }

  } else if (op < 70) {
    // 30% chance: SET string operation
    const char* keys[] = {
      "wifi_ssid",
      "mqtt_host",
      "device_name",
      "sensor1_name",
      "log_level",
      "hostname",
      "ntp_server",
      "temp_unit"};
    const char* values[] = {
      "Network1",
      "broker.local",
      "ESP Device",
      "TempSensor",
      "INFO",
      "my-esp32",
      "time.cloudflare.com",
      "F"};
    int idx = random(0, 8);
    auto result = config.set(keys[idx], values[idx]);
    Serial.printf("[SET] %s = %s (result: %s)\n",
                  keys[idx],
                  values[idx],
                  result ? "OK" : "FAILED");

  } else if (op < 85) {
    // 15% chance: SET bool operation
    const char* keys[] = {
      "wifi_enable",
      "mqtt_enable",
      "ntp_enable",
      "web_enable",
      "relay1_enable",
      "sensor1_enbl",
      "log_enable"};
    const char* key = keys[random(0, 7)];
    bool value = random(0, 2);
    auto result = config.set(key, value);
    Serial.printf("[SET_BOOL] %s = %s (result: %s)\n",
                  key,
                  value ? "true" : "false",
                  result ? "OK" : "FAILED");

  } else {
    // 15% chance: UNSET operation
    const char* keys[] = {
      "pwm1_duty",
      "pwm2_duty",
      "temp_offset",
      "pres_offset",
      "hum_offset"};
    const char* key = keys[random(0, 5)];
    auto result = config.unset(key);
    Serial.printf("[UNSET] %s (result: %s)\n",
                  key,
                  result ? "OK" : "FAILED");
  }

  operationCount++;

  // Add small delay to avoid overwhelming the serial output
  delay(random(100, 500));
}
