#include <MycilaConfig.h>

Mycila::Config config;
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

  config.listen([](const char* key, const String& newValue) {
    Serial.printf("(listen) '%s' => '%s'\n", key, newValue.c_str());
  });

  config.listen([]() {
    Serial.println("(restored)");
  });

  // begin()

  config.begin();

  // configure()

  config.configure("key1", "false");
  config.configure("key2", "");
  config.configure("key3");
  config.configure("key4", "foo");
  config.configure("key5", "baz");

  // tests

  assertEquals(config.get("key1"), "false");
  assertEquals(config.get("key2"), "");
  assertEquals(config.get("key3"), "");

  // set key
  assert(config.set("key1", "true"));
  assertEquals(config.get("key1"), "true");
  assert(prefs.isKey("key1"));

  // set key to same value => no change
  assert(!config.set("key1", "true"));

  // cache stored key
  assertEquals(config.get("key4"), "bar"); // load key and cache

  // set key to same value => no change
  assert(!config.set("key4", "bar"));

  // set stored key to default value
  assert(config.set("key4", "foo"));
  assertEquals(config.get("key4"), "foo");
  assert(prefs.isKey("key4"));

  // set stored key to other value
  assert(config.set("key4", "bar"));
  assertEquals(config.get("key4"), "bar");

  // unset stored key
  assert(config.unset("key4"));
  assert(!prefs.isKey("key4"));
  assertEquals(config.get("key4"), "foo");

  // unset non-existing key => noop
  assert(!config.unset("key4"));
  assertEquals(config.get("key4"), "foo");

  // set un-stored to default value => no change
  assert(!config.set("key5", "baz"));

  // unset non stored key => noop
  assert(!config.unset("key5"));

  config.backup(Serial);

  config.set("key1", "value1");
  config.set("key2", "value2");

  config.backup(Serial);

  config.restore("key1=\nkey2=\nkey3=value3\nkey4=foo\n");
}

void loop() {
  vTaskDelete(NULL);
}
