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

// 1 = at start-up, redraw a test label after each step and log which step stops the panel answering.
#define BRINGUP_STEPS 1

void setup() {
  Serial.begin(115200);
#if BRINGUP_STEPS
  uint32_t t = millis();
  while (!Serial && millis() - t < 6000) delay(10);  // time to open the Serial Monitor
#endif
  delay(300);
  LOG("Sobergotchi " FIRMWARE_VERSION);
#if BRINGUP_STEPS
  // Draw the real home screen after each start-up step; the label along the bottom names the step.
  store::begin();
  display::begin();
  app::bringupStep("1 display only");
  hw::begin();
  app::bringupStep("2 after buttons");
  hw::readBattery();
  LOG("battery %d mV", (int)(hw::batteryVolts() * 1000));
  app::bringupStep("3 after battery");
  exchange::begin();
  app::bringupStep("4 after bluetooth");
  LOG("steps done, starting the app");
#else
  store::begin();      // before Bluetooth: first start uses the hardware random generator
  LOG("storage ok, %d friends", store::friendCount);
  hw::begin();
  LOG("inputs ok");
  display::begin();
  LOG("display ok");
  exchange::begin();
  LOG("bluetooth ok");
#endif
  app::begin();
  LOG("home screen drawn");
}

void loop() {
  app::loop();
}
