#include <Arduino.h>
#include <Stream.h>
#include <ArduinoLog.h>  // https://github.com/thijse/Arduino-Log/
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#include <sstream>

#define LOGLEVEL LOG_LEVEL_NOTICE
//#define LOGLEVEL LOG_LEVEL_WARNING

//#define USE_SOFT_RESET 1

#define DELAY_INTERVAL (15 * 1000)
#define RETRY_INTERVAL (1 * 1000)

#define MAX_JSON_SIZE 1024
#define MAX_DEVICE_NAME 128

extern void setupWiFi();
extern void setupMQTT();
extern void loopClient();
extern int loopMQTT();
extern int publishJson(JsonObject &jsonObject);

extern void setupTFT(int size);
extern void displayTFT(const char *format, ...);

extern void setEventHandler(void (*handler)(JsonObject &objec));
extern void setResetFunc(void (*func)(void));

void setupDevices(char deviceNameBuffer[])
{
  strcpy(deviceNameBuffer, "Monitor");
}

#if 0
void updateDevices(JsonObject &jsonObject)
{
  {
    delay(RETRY_INTERVAL);
  }
}
#endif

char deviceNameBuffer[MAX_DEVICE_NAME];

void resetESP()
{
#ifdef ESP8266
  ESP.reset();
#endif // ESP8266
#ifdef ESP32
  ESP.restart();
#endif // ESP32
}

#ifdef USE_SOFT_RESET
void softReset()
{
  StaticJsonBuffer<MAX_JSON_SIZE> errorJsonBuffer;
  JsonObject &errorJsonObject = errorJsonBuffer.createObject();

  Log.notice("softReset()\n");
  errorJsonObject["Reset"] = "soft";
  publishJson(errorJsonObject);
  resetESP();
}
#endif

void displayMessage(JsonObject &jsonObject)
{
  StaticJsonBuffer<MAX_JSON_SIZE> tagJsonBuffer;
  JsonObject &tagJsonObject = tagJsonBuffer.createObject();

  tagJsonObject["Type"] = 1;
  tagJsonObject["Timestamp"] = 1;
  tagJsonObject["MAC"] = 1;

  std::stringstream ss;

  // Type, Timestamp and MAC
  for (auto tagkv : tagJsonObject)
  {
    ss << tagkv.key << ": " << jsonObject[tagkv.key].as<char *>() << "\n";
  }

  // Other than Type, Timestamp and MAC
  for (auto kv : jsonObject)
  {
    if (tagJsonObject[kv.key] == NULL)
    {
      ss << kv.key << ": " << kv.value.as<char *>() << "\n";
    }
  }
  Log.notice("ss=%s\n", ss.str().c_str());

  displayTFT(ss.str().c_str());
}

void controlEventHandler(JsonObject &jsonObject)
{
#ifdef USE_SOFT_RESET
  const char *resetMethod = (const char *)jsonObject["Reset"];

  if (resetMethod != NULL)
  {
    if (strcmp(resetMethod, "soft") == 0)
    {
      Log.notice("softReset\n");
      softReset();
    }
  }
#endif
  displayMessage(jsonObject);
}

char *getDeviceName()
{
  return deviceNameBuffer;
}

void setup()
{
  Serial.begin(115200);
  Log.begin(LOGLEVEL, &Serial);
  delay(2000);

  setupTFT(1);

  displayTFT("Display initialized.");

  setupDevices(deviceNameBuffer);
  displayTFT("Device setup done.");


  setupWiFi();
  displayTFT("WiFi setup done.");

  setupMQTT();
  displayTFT("MQTT setup done.");

  setEventHandler(controlEventHandler);
#ifdef USE_SOFT_RESET
  setResetFunc(softReset);
#endif // USE_HARD_RESET

  displayTFT("Setup done.");
  delay(5000);
}

unsigned long lasttime;

void loop()
{
  unsigned long time = millis();

  if ((time - lasttime) < DELAY_INTERVAL)
  {
    loopClient();
    delay(1000);
    return;
  }
  else
  {
    lasttime = time;
  }
  Log.notice("freesize=%d\n", ESP.getFreeHeap());
}