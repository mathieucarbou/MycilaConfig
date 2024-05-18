#include <MycilaConfig.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PWD "wifi_pwd"

Mycila::Config config;

uint8_t getLogLevel() { return config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  config.begin();

  config.configure(KEY_DEBUG_ENABLE, "false");
  config.configure(KEY_WIFI_SSID);
  config.configure(KEY_WIFI_PWD);
}

void loop() {
  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_INFO);

  config.setBool(KEY_DEBUG_ENABLE, !config.getBool(KEY_DEBUG_ENABLE));

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_DEBUG);

  delay(5000);
}
