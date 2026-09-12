#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Arduino_GC9B72.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoOblique24pt7b.h>


// XIAO ESP32C3 only breaks out GPIO 2,3,4,5,6,7,8,9,10,20,21 (D0-D10).
// GPIO 11-14 (used by the S3 example this driver ships with) do not exist
// on this board's header, so the panel wiring must use these instead.
#define TFT_SCLK 8   // D8
#define TFT_MOSI 10  // D10
#define TFT_CS   7   // D5
#define TFT_DC   21  // D6
#define TFT_RST  20  // D7
#define TFT_BL   6   // D4

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_GC9B72(bus, TFT_RST, 0 /*rotation*/, false /*IPS*/, 360, 360);

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin(40000000);
  gfx->fillScreen(RGB565_BLACK);

  gfx->setCursor(10, 170);
  gfx->setTextColor(RGB565_GREEN);
  gfx->setFont(&FreeMonoOblique24pt7b);

  
  //gfx->println("off to the       Races!");
  
  gfx->drawPixel(359,180,RGB565_RED);
  gfx->fillCircle(180,180,179,RGB565_CYAN);
  gfx->fillCircle(180,180,160,RGB565_BLACK);

}

void loop() {
  

}
