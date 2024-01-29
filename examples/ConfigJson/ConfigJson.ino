#include <ArduinoJson.h>
#include <MycilaConfig.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PWD "wifi_pwd"

uint8_t getLogLevel() { return Mycila::Config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Mycila::Config.begin();

  Mycila::Config.configure(KEY_DEBUG_ENABLE, "false");
  Mycila::Config.configure(KEY_WIFI_SSID);
  Mycila::Config.configure(KEY_WIFI_PWD);
}

void loop() {
  JsonDocument doc;
  Mycila::Config.toJson(doc.to<JsonObject>());
  serializeJson(doc, Serial);
  Serial.println();

  Serial.println(Mycila::Config.backup());

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_INFO);

  Mycila::Config.setBool(KEY_DEBUG_ENABLE, !Mycila::Config.getBool(KEY_DEBUG_ENABLE));

  assert(getLogLevel() == ARDUHAL_LOG_LEVEL_DEBUG);

  delay(5000);
}
