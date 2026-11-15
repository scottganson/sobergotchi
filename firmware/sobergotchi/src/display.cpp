#include "display.h"

#include <GxEPD2_BW.h>
#include <SPI.h>

#include "config.h"
#include "ui/renderer.h"

namespace display {

static GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> epd(
    GxEPD2_213_GDEY0213B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
static int partials = 0;
static bool awake = false;

void begin() {
  pinMode(PIN_EPD_POWER, OUTPUT);
  digitalWrite(PIN_EPD_POWER, HIGH);
  delay(50);  // let the panel supply settle before the reset pulse
  SPI.setPins(PIN_SPI_UNUSED_MISO, PIN_EPD_SCK, PIN_EPD_MOSI);
  epd.init(0, true, 2, false);
  epd.setRotation(0);  // portrait, 122 x 250
  epd.setTextWrap(false);
  awake = true;
}

void show(uint8_t screenId, UiData& data, bool full) {
  pinMode(PIN_EPD_DC, OUTPUT);  // the battery reading borrows this pin
  if (!full && partials >= FULL_REFRESH_EVERY) full = true;
  if (full) {
    epd.setFullWindow();
    partials = 0;
  } else {
    epd.setPartialWindow(0, 0, epd.width(), epd.height());
    partials++;
  }
  uint32_t t0 = millis();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    ui::draw(epd, screenId, data);
  } while (epd.nextPage());
  LOG("screen %d %s refresh %lu ms", screenId, full ? "full" : "partial", (unsigned long)(millis() - t0));
  awake = true;
}

void sleep() {
  if (awake) epd.hibernate();
  awake = false;
}

}  // namespace display
