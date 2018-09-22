#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#define TFT_CS     D4
#define TFT_RST    -1  // you can also connect this to the Arduino reset
// in which case, set this #define pin to -1!
#define TFT_DC     D3

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

#include <SPI.h>
#include <Wire.h>

#include <ArduinoLog.h>


#define TFT_SETUP_MESSAGE "TFT setup"
#define MAX_TFT_MESSAGE_SIZE 1024

void clearTFT()
{
  tft.fillScreen(ST7735_WHITE);
  tft.setCursor(0, 5); // adjusting an starting offset 
}

void displayTFT(const char *format, ...)
{
  char buf[MAX_TFT_MESSAGE_SIZE];

  va_list va;
  va_start(va, format);
  vsprintf(buf, format, va);
  va_end(va);

  clearTFT();
  tft.println(buf);
}

void setupTFT(int size)
{
  tft.initR(INITR_144GREENTAB);
  tft.setTextWrap(true);
  tft.fillScreen(ST7735_WHITE);

  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_BLACK);

  tft.setTextSize(size);

  displayTFT(TFT_SETUP_MESSAGE);
}


