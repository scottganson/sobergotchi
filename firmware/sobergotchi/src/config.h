// Sobergotchi hardware and behaviour settings.
// Board: Seeed Studio XIAO ePaper Display Board EN05 (XIAO nRF52840 Plus)
// Panel: GDEY0213B74, 2.13" 122x250 black/white, SSD1680 controller
#pragma once
#include <Arduino.h>

#define FIRMWARE_VERSION "1.21-world18"

// 1 = rainy days turn to snow in December, January and February. Set 0 for places without snowy winters.
#define WINTER_SNOW 1

// ---------------------------------------------------------------------------
// Defaults used the very first time the device starts (change later in Settings)
// ---------------------------------------------------------------------------
#define DEFAULT_NAME        "Scott"
#define DEFAULT_PHONE       ""          // digits only
#define DEFAULT_SOBER_YEAR  2024
#define DEFAULT_SOBER_MONTH 8
#define DEFAULT_SOBER_DAY   19

// ---------------------------------------------------------------------------
// Pins. Arduino pin numbers for the "Seeed XIAO nRF52840 Plus" board package.
// Display pins come from Seeed's EN05 board definition (Seeed_GFX2).
// ---------------------------------------------------------------------------
#define PIN_EPD_CS      7    // D7
#define PIN_EPD_DC      35   // D16 (P0.31) - shared with the battery sense input
#define PIN_EPD_RST     30   // D11 (P0.15)
#define PIN_EPD_BUSY    3    // D3
#define PIN_EPD_SCK     8    // D8
#define PIN_EPD_MOSI    10   // D10
#define PIN_EPD_POWER   6    // D6, high = panel powered
#define PIN_SPI_UNUSED_MISO 19 // the panel has no MISO; park SPI MISO on an unconnected pin (not D9!)

// Buttons. Physically, left to right with the screen facing you: D2, D9, D1.
// Up / down / enter layout (2026-11-16):
#define PIN_KEY1        2    // left,   D2, active low: UP    (previous item, value +1)
#define PIN_KEY2        1    // right,  D1, active low: ENTER (select / next field; hold = cancel or save in editors)
#define PIN_KEY3        9    // middle, D9, active low: DOWN  (next item, value -1)

// Battery, from Seeed's Ex05 schematic: the battery goes through a switch (on while D12 is high) and a
// 10k/10k divider to D0/A0. Reading the XIAO's own battery pin (P0.31) is not possible here: it is the display DC line.
#define PIN_BAT_ADC     0              // D0 / A0 (P0.02), half the battery voltage while the switch is on
#define PIN_BAT_ADC_EN  31             // D12 (P0.19), high = battery divider connected
#define PIN_BAT_ENABLE  VBAT_ENABLE    // 14 = P0.14, the XIAO's own divider; kept LOW as before
#define BATTERY_SENSE   1              // 0 = no reading, show a nominal 4.20 V
#define PIN_CHARGING    23             // P0.17, LOW while the charger is charging

// ---------------------------------------------------------------------------
// Behaviour
// ---------------------------------------------------------------------------
#define IDLE_TIMEOUT_MS        30000UL  // back to the home screen after this long without a press
#define LONG_PRESS_MS          700UL
#define POPUP_MS               6000UL   // how long "Friend added" / rock-paper-scissors stay up
#define ADD_FRIEND_MS          30000UL  // Friends > Add friend: the radio is on this long, always, so the
                                        // other person has time to start theirs too
#define ADD_FRIEND_HOLD_MS     2000UL   // hold enter + up together this long to start Add friend from any screen;
                                        // the radio then stays on only while they are held
#define ADD_FRIEND_HOLD_MAX_MS 120000UL // ...but never more than 2 minutes (a button stuck in a pocket)
#define ADD_FRIEND_MIN_RSSI    (-80)    // reaches across a table (about 1-2 m), not just back to back
#define MAX_FRIENDS            200
#define NAME_LEN               12
#define PHONE_LEN              15
#define BLE_COMPANY_ID         0xFFFF   // test company ID, fine for a personal project

// Start-up and screen messages on Tools > Serial Monitor (115200 baud). Costs nothing when no computer is connected.
#define DEBUG_LOG 1
#if DEBUG_LOG
#define LOG(...) do { if (Serial) { Serial.printf(__VA_ARGS__); Serial.println(); } } while (0)
#else
#define LOG(...) do { } while (0)
#endif
