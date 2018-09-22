#include <Arduino.h>
#include <Stream.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <time.h>
#else
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#endif
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <ArduinoLog.h>
#include <ArduinoJson.h>

// TIME
#define TIMEDIFF 9 /* JST */
#define JST 3600 * TIMEDIFF
// NTP servers
#define NTP_SERVERS "ntp.nict.jp", "ntp.jst.mfeed.ad.jp"

byte macAddr[6];
char macAddrString[2 + 2 * 6 + 1];

// IP address
IPAddress ipAddr;

// WiFi Manager
WiFiManager wifiManager;

char *getTimestamp()
{
  static char timestampString[32];

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  int timediff = TIMEDIFF;

  sprintf(timestampString, "%04d-%02d-%02dT%02d:%02d:%02d+%02d:00", 
    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
    tm->tm_hour, tm->tm_min, tm->tm_sec, timediff);

  return (timestampString);
}

char *getMacAddress()
{
  return macAddrString;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
#ifdef ESP8266
  Log.notice("configModeCallback():%s,%s:\n", WiFi.softAPIP().toString().c_str(), myWiFiManager->getConfigPortalSSID().c_str());
#else
  Log.notice("configModeCallback():%s,%s:\n", WiFi.softAPIP(), myWiFiManager->getConfigPortalSSID());
#endif
}

void setupWiFi()
{
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);

  // first parameter is name of access point, second is the password
  wifiManager.autoConnect();

  // Connect done
  Log.notice("WiFi Connected.\n");

  // setup NTP
  configTime(JST, 0, NTP_SERVERS);

  // get MAC address
  WiFi.macAddress(macAddr);

  sprintf(macAddrString, "0x%x%x%x%x%x%x%x%x%x%x%x%x",
          (macAddr[0] >> 4) & 0xf, macAddr[0] & 0xf,
          (macAddr[1] >> 4) & 0xf, macAddr[1] & 0xf,
          (macAddr[2] >> 4) & 0xf, macAddr[2] & 0xf,
          (macAddr[3] >> 4) & 0xf, macAddr[3] & 0xf,
          (macAddr[4] >> 4) & 0xf, macAddr[4] & 0xf,
          (macAddr[5] >> 4) & 0xf, macAddr[5] & 0xf);
  Log.notice("macAddress: %s\n", macAddrString);

  // get IP address
  ipAddr = WiFi.localIP();
  Log.notice("ipAddr: %d.%d.%d.%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
}


