#include <ArduinoLog.h>

void setupDeviceName(char deviceNameBuffer[], const char* deviceName) {
  Log.notice("+setupDeviceName %s\n", deviceNameBuffer);
   if ( deviceNameBuffer[0] != '\0' ) {
    strcat(deviceNameBuffer, "+");
  }
  strcat(deviceNameBuffer, deviceName);
  Log.notice("-setupDeviceName %s\n", deviceNameBuffer);
}
