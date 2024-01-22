#define MYCILA_LOGGER_CUSTOM_LEVEL

#include <MycilaConfig.h>
#include <MycilaLogger.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PWD "wifi_pwd"

uint8_t Mycila::LoggerClass::getLevel() const { return Mycila::Config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Mycila::Logger.forwardTo(&Serial);
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

  Mycila::Logger.debug("APP", "Debug log");
  Mycila::Logger.info("APP", "Info log");

  Mycila::Config.setBool(KEY_DEBUG_ENABLE, !Mycila::Config.getBool(KEY_DEBUG_ENABLE));

  Mycila::Logger.debug("APP", "Debug log");
  Mycila::Logger.info("APP", "Info log");

  delay(5000);
}
