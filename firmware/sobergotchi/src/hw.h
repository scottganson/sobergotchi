// Hardware: wake-up events, buttons, reed switch, battery, low-power waiting and the clock.
#pragma once
#include <Arduino.h>

enum Event : uint8_t {
  EV_NONE = 0,
  EV_KEY1, EV_KEY2, EV_KEY3,     // short presses
  EV_KEY2_LONG,                  // KEY2 held
  EV_REED,                       // a magnet closed the reed switch
  EV_TIMEOUT,                    // nothing happened before the timeout
};

namespace hw {

void begin();
// Sleeps (System ON, ~µA) until a button, the reed switch or the timeout. Returns what woke us.
Event wait(uint32_t timeoutMs);

// Battery
int batteryPercent();
void readBattery();   // takes about 15 ms
bool batteryStale();
float batteryVolts();  // cached
bool charging();     // USB connected and the charger is charging
bool usbPower();

// Clock: local time in seconds since 1970-01-01, kept by the 32 kHz crystal while asleep.
uint32_t now();
void setNow(uint32_t secs);
int32_t today();     // day number
uint32_t secondsToMidnight();
uint32_t buildTime();  // when this firmware was compiled (local time of the build PC)

}  // namespace hw
