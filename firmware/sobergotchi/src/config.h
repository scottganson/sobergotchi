// Sobergotchi hardware and behaviour settings.
// Board: Seeed Studio XIAO ePaper Display Board EN05 (XIAO nRF52840 Plus)
// Panel: GDEY0213B74, 2.13" 122x250 black/white, SSD1680 controller
#pragma once
#include <Arduino.h>

#define FIRMWARE_VERSION "1.0"

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

#define PIN_KEY1        1    // D1, active low: back / previous / minus
#define PIN_KEY2        2    // D2, active low: select / next field (hold = back)
#define PIN_KEY3        9    // D9, active low: forward / next / plus
#define PIN_REED        0    // D0, reed switch to GND (closes near a magnet)

#define PIN_BAT_SENSE   PIN_VBAT       // 35 = P0.31 through a 1M/510k divider
#define PIN_BAT_ENABLE  VBAT_ENABLE    // 14 = P0.14, keep LOW (Seeed: protects P0.31 while charging)
#define PIN_CHARGING    23             // P0.17, LOW while the charger is charging

// ---------------------------------------------------------------------------
// Behaviour
// ---------------------------------------------------------------------------
#define IDLE_TIMEOUT_MS        30000UL  // back to the home screen after this long without a press
#define LONG_PRESS_MS          700UL
#define POPUP_MS               6000UL   // how long "Friend added" / rock-paper-scissors stay up
#define FULL_REFRESH_EVERY     10       // partial refreshes before a cleaning full refresh
#define EXCHANGE_MS            4000UL   // how long to swap cards after the reed closes
#define EXCHANGE_MIN_RSSI      (-60)    // ignore devices further than a few centimetres away
#define MAX_FRIENDS            200
#define NAME_LEN               12
#define PHONE_LEN              15
#define BLE_COMPANY_ID         0xFFFF   // test company ID, fine for a personal project
