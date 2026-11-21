#include "hw.h"

#include "config.h"
#include "datemath.h"

namespace hw {

static SemaphoreHandle_t wakeSem;
static volatile uint8_t pending;  // bit per input that fired
enum { B_KEY1 = 1, B_KEY2 = 2, B_KEY3 = 4 };

static void isrKey1() { pending |= B_KEY1; BaseType_t w = pdFALSE; xSemaphoreGiveFromISR(wakeSem, &w); portYIELD_FROM_ISR(w); }
static void isrKey2() { pending |= B_KEY2; BaseType_t w = pdFALSE; xSemaphoreGiveFromISR(wakeSem, &w); portYIELD_FROM_ISR(w); }
static void isrKey3() { pending |= B_KEY3; BaseType_t w = pdFALSE; xSemaphoreGiveFromISR(wakeSem, &w); portYIELD_FROM_ISR(w); }

// ---- clock ----------------------------------------------------------------
static uint32_t baseSecs;       // local time at baseTick
static TickType_t baseTick;
static uint32_t subTicks;       // leftover ticks below one second

uint32_t now() {
  TickType_t t = xTaskGetTickCount();
  uint32_t elapsed = (uint32_t)(t - baseTick) + subTicks;  // wraps safely; we wake at least hourly
  baseSecs += elapsed / configTICK_RATE_HZ;
  subTicks = elapsed % configTICK_RATE_HZ;
  baseTick = t;
  return baseSecs;
}

void setNow(uint32_t secs) {
  baseTick = xTaskGetTickCount();
  subTicks = 0;
  baseSecs = secs;
}

int32_t today() { return (int32_t)(now() / 86400UL); }

uint32_t secondsToMidnight() { return 86400UL - now() % 86400UL; }

static int monthFromName(const char* s) {
  static const char names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  for (int i = 0; i < 12; i++)
    if (!strncmp(s, names + i * 3, 3)) return i + 1;
  return 1;
}

uint32_t buildTime() {
  const char* d = __DATE__;  // "Nov 15 2026"
  const char* t = __TIME__;  // "07:22:03"
  int32_t day = daysFromCivil(atoi(d + 7), monthFromName(d), atoi(d + 4));
  return (uint32_t)day * 86400UL + atoi(t) * 3600UL + atoi(t + 3) * 60UL + atoi(t + 6);
}

// ---- battery --------------------------------------------------------------
static float lastVolts = 0;
static uint32_t lastRead = 0;

void readBattery() {
#if !BATTERY_SENSE
  lastVolts = 4.20f;
  lastRead = millis();
  return;
#endif
  pinMode(PIN_BAT_ADC, INPUT);
  digitalWrite(PIN_BAT_ADC_EN, HIGH);  // connect the divider
  analogReference(AR_INTERNAL_2_4);
  analogReadResolution(12);
  delay(10);                           // Seeed: let the divider settle before sampling
  analogRead(PIN_BAT_ADC);             // first sample after switching references is thrown away
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(PIN_BAT_ADC);
  digitalWrite(PIN_BAT_ADC_EN, LOW);   // disconnect again so the divider does not drain the battery
  analogReference(AR_DEFAULT);
  analogReadResolution(10);
  lastVolts = (sum / 8.0f) * (2.4f / 4095.0f) * 2.0f;
  LOG("battery raw %d = %d mV%s", (int)(sum / 8), (int)(lastVolts * 1000), charging() ? " (charging)" : "");
  lastRead = millis();
}

float batteryVolts() {
  if (lastRead == 0) readBattery();  // first use only; afterwards the app refreshes it between screens
  return lastVolts;
}

bool batteryStale() { return lastRead == 0 || millis() - lastRead > 10UL * 60UL * 1000UL; }

int batteryPercent() {
  static const float v[] = {4.15f, 4.05f, 3.97f, 3.90f, 3.84f, 3.80f, 3.76f, 3.73f, 3.70f, 3.65f, 3.50f, 3.30f};
  static const uint8_t p[] = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 5, 0};
  float x = batteryVolts();
  if (x >= v[0]) return 100;
  for (int i = 1; i < 12; i++)
    if (x >= v[i]) return p[i] + (int)((x - v[i]) / (v[i - 1] - v[i]) * (p[i - 1] - p[i]));
  return 0;
}

bool usbPower() { return NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk; }
bool charging() { return usbPower() && digitalRead(PIN_CHARGING) == LOW; }

// ---- inputs ---------------------------------------------------------------
void begin() {
  wakeSem = xSemaphoreCreateBinary();
  pinMode(PIN_BAT_ADC_EN, OUTPUT);
  digitalWrite(PIN_BAT_ADC_EN, LOW);  // battery divider off until a reading is taken
  pinMode(PIN_BAT_ENABLE, OUTPUT);
  digitalWrite(PIN_BAT_ENABLE, LOW);  // Seeed: keep low so P0.31 never sees the raw battery while charging
  pinMode(PIN_CHARGING, INPUT);
  const uint8_t pins[] = {PIN_KEY1, PIN_KEY2, PIN_KEY3};
  for (uint8_t p : pins) pinMode(p, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_KEY1), isrKey1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_KEY2), isrKey2, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_KEY3), isrKey3, FALLING);
}

// Enter (KEY2) and up (KEY1) are both down. Held together for ADD_FRIEND_HOLD_MS: Add friend, straight away, while they
// are still held (the radio stays on until they are let go). Let go sooner: nothing, once both are
// up, so the combo never also counts as an enter or an up.
static Event comboHold() {
  const uint32_t t0 = millis();
  while (digitalRead(PIN_KEY1) == LOW && digitalRead(PIN_KEY2) == LOW) {
    if (millis() - t0 >= ADD_FRIEND_HOLD_MS) return EV_ADD_FRIEND;
    delay(10);
  }
  while (digitalRead(PIN_KEY1) == LOW || digitalRead(PIN_KEY2) == LOW) delay(10);
  return EV_NONE;
}

static bool comboDown() { return digitalRead(PIN_KEY1) == LOW && digitalRead(PIN_KEY2) == LOW; }
bool comboHeld() { return comboDown(); }
bool downHeld() { return digitalRead(PIN_KEY3) == LOW; }

void flushKeys() {
  delay(30);
  noInterrupts();
  pending = 0;
  interrupts();
  xSemaphoreTake(wakeSem, 0);
}

static Event classify(uint8_t bits) {
  if (bits & B_KEY2) {
    delay(20);
    if (digitalRead(PIN_KEY2) == LOW) {
      uint32_t t0 = millis();
      while (digitalRead(PIN_KEY2) == LOW) {
        if (comboDown()) return comboHold();
        if (millis() - t0 >= LONG_PRESS_MS) {
          while (digitalRead(PIN_KEY2) == LOW) {
            if (comboDown()) return comboHold();  // up joined late: still the combo, not enter
            delay(10);
          }
          return EV_KEY2_LONG;
        }
        delay(10);
      }
      return EV_KEY2;
    }
  }
  if (bits & B_KEY1) {
    delay(20);
    if (digitalRead(PIN_KEY1) == LOW) {
      while (digitalRead(PIN_KEY1) == LOW) {
        if (comboDown()) return comboHold();  // enter joined: the combo, not up
        delay(10);
      }
      return EV_KEY1;
    }
  }
  if (bits & B_KEY3) {
    delay(20);
    if (digitalRead(PIN_KEY3) == LOW) {
      // a short press of down selects; held for CALM_HOLD_MS it opens the calm screen straight away (still held)
      const uint32_t t0 = millis();
      while (digitalRead(PIN_KEY3) == LOW) {
        if (millis() - t0 >= CALM_HOLD_MS) return EV_CALM;
        delay(10);
      }
      return EV_KEY3;
    }
  }
  return EV_NONE;
}

Event wait(uint32_t timeoutMs) {
  if (timeoutMs > 3600000UL) timeoutMs = 3600000UL;  // keeps tick maths in range; callers loop
  uint32_t start = millis();
  for (;;) {
    noInterrupts();
    uint8_t bits = pending;
    pending = 0;
    interrupts();
    if (bits) {
      Event e = classify(bits);
      delay(15);
      pending = 0;  // drop bounces from the press we just handled
      if (e != EV_NONE) return e;
    }
    uint32_t spent = millis() - start;
    if (spent >= timeoutMs) return EV_TIMEOUT;
    // Block here: FreeRTOS tickless idle puts the chip to sleep until an interrupt or the timeout.
    xSemaphoreTake(wakeSem, pdMS_TO_TICKS(timeoutMs - spent));
    now();  // keep the clock base fresh
  }
}

}  // namespace hw
