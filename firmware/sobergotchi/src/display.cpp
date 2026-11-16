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
  if (digitalRead(PIN_EPD_BUSY) == HIGH) {
    // Panel stuck busy (it ignored the wake-up reset): cut its power, start it fresh, force a full refresh.
    LOG("screen %d: panel stuck busy, power-cycling it", screenId);
    digitalWrite(PIN_EPD_POWER, LOW);
    delay(200);
    digitalWrite(PIN_EPD_POWER, HIGH);
    delay(50);
    epd.init(0, true, 2, false);
    epd.setRotation(0);
    epd.setFullWindow();
    partials = 0;
    full = true;
    LOG("screen %d: busy pin %d after power-cycle", screenId, digitalRead(PIN_EPD_BUSY));
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

bool probe(const char* label) {
  pinMode(PIN_EPD_DC, OUTPUT);
  LOG("probe '%s': busy pin %d before", label, digitalRead(PIN_EPD_BUSY));
  uint32_t t0 = millis();
  epd.setFullWindow();
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.setTextColor(GxEPD_BLACK);
    epd.setTextSize(2);
    epd.setCursor(6, 100);
    epd.print(label);
  } while (epd.nextPage());
  uint32_t ms = millis() - t0;
  LOG("probe '%s': %lu ms %s", label, (unsigned long)ms, ms > 9000 ? "FAILED" : "ok");
  return ms <= 9000;
}

void sleep() {
  // powerOff, not hibernate: waking from deep sleep needs a reset pulse, and the panel never came back from it.
  if (awake) epd.powerOff();
  awake = false;
}

}  // namespace display
