// Sobergotchi - a pocket companion for people in 12 step recovery.
// Board: Seeed XIAO ePaper Display Board EN05 (choose "Seeed XIAO nRF52840 Plus" in Tools > Board).
// Libraries: GxEPD2 (Jean-Marc Zingg), Adafruit GFX. Bluetooth and flash storage come with the board package.
// Screens are defined in ../../screens/*.sml; run tools/build_ui.py after changing them.

#include "src/config.h"
#include "src/hw.h"
#include "src/store.h"
#include "src/display.h"
#include "src/exchange.h"
#include "src/app.h"

void setup() {
  store::begin();      // before Bluetooth: first start uses the hardware random generator
  hw::begin();
  display::begin();
  exchange::begin();
  app::begin();
}

void loop() {
  app::loop();
}
