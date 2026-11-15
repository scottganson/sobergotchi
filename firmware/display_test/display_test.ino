// Sobergotchi display and button test for the XIAO ePaper EN05 board.
// Upload it, open Tools > Serial Monitor at 115200 baud, and press each button.
// The screen should show a frame and "Display OK". Every press redraws the screen
// with the name of the button and prints it to the Serial Monitor.
#include <GxEPD2_BW.h>
#include <SPI.h>

// Set to 1 to try GxEPD2's older 2.13" B74 driver if the screen stays noisy.
#define USE_OLD_B74_DRIVER 0

#define PIN_EPD_CS    7    // D7
#define PIN_EPD_DC    35   // D16 (P0.31)
#define PIN_EPD_RST   30   // D11 (P0.15)
#define PIN_EPD_BUSY  3    // D3
#define PIN_EPD_SCK   8    // D8
#define PIN_EPD_MOSI  10   // D10
#define PIN_EPD_POWER 6    // D6, high = panel powered
#define PIN_NO_MISO   19   // park SPI MISO away from the buttons

const uint8_t KEYS[] = {1, 2, 9, 0};  // KEY1 = D1, KEY2 = D2, KEY3 = D9, reed = D0
const char* KEY_NAMES[] = {"KEY1", "KEY2", "KEY3", "REED"};

#if USE_OLD_B74_DRIVER
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> epd(GxEPD2_213_B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
const char* DRIVER = "213_B74";
#else
GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> epd(GxEPD2_213_GDEY0213B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
const char* DRIVER = "GDEY0213B74";
#endif

int presses = 0;

void draw(const char* line1, const char* line2, bool full) {
  if (full) epd.setFullWindow();
  else epd.setPartialWindow(0, 0, epd.width(), epd.height());
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.drawRect(0, 0, epd.width(), epd.height(), GxEPD_BLACK);
    epd.drawRect(3, 3, epd.width() - 6, epd.height() - 6, GxEPD_BLACK);
    epd.fillRect(8, 8, epd.width() - 16, 40, GxEPD_BLACK);
    epd.setTextColor(GxEPD_WHITE);
    epd.setTextSize(2);
    epd.setCursor(14, 20);
    epd.print("Display");
    epd.setTextColor(GxEPD_BLACK);
    epd.setCursor(12, 70);
    epd.print(line1);
    epd.setTextSize(1);
    epd.setCursor(12, 100);
    epd.print(line2);
    epd.setCursor(12, 120);
    epd.print("driver ");
    epd.print(DRIVER);
    epd.setCursor(12, 135);
    epd.print("presses ");
    epd.print(presses);
    for (int i = 0; i < 4; i++) epd.fillRect(12 + i * 26, 160, 20, 20, (i % 2) ? GxEPD_BLACK : GxEPD_WHITE), epd.drawRect(12 + i * 26, 160, 20, 20, GxEPD_BLACK);
    epd.fillCircle(epd.width() / 2, 215, 18, GxEPD_BLACK);
  } while (epd.nextPage());
}

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 4000) delay(10);
  Serial.println();
  Serial.println("Sobergotchi display test");
  Serial.print("driver: ");
  Serial.println(DRIVER);

  for (uint8_t k : KEYS) pinMode(k, INPUT_PULLUP);
  pinMode(PIN_EPD_POWER, OUTPUT);
  digitalWrite(PIN_EPD_POWER, HIGH);
  delay(50);
  pinMode(PIN_EPD_BUSY, INPUT);
  Serial.print("BUSY pin after power on: ");
  Serial.println(digitalRead(PIN_EPD_BUSY));

  SPI.setPins(PIN_NO_MISO, PIN_EPD_SCK, PIN_EPD_MOSI);
  epd.init(115200, true, 2, false);  // 115200 = GxEPD2 prints busy timings to Serial
  epd.setRotation(0);
  Serial.print("size: ");
  Serial.print(epd.width());
  Serial.print(" x ");
  Serial.println(epd.height());
  uint32_t t0 = millis();
  draw("OK", "full refresh", true);
  Serial.print("full refresh took ms: ");
  Serial.println(millis() - t0);
  Serial.println("press a button");
}

void loop() {
  for (int i = 0; i < 4; i++) {
    if (digitalRead(KEYS[i]) == LOW) {
      presses++;
      Serial.print(KEY_NAMES[i]);
      Serial.println(" pressed");
      uint32_t t0 = millis();
      draw(KEY_NAMES[i], "partial refresh", false);
      Serial.print("partial refresh took ms: ");
      Serial.println(millis() - t0);
      while (digitalRead(KEYS[i]) == LOW) delay(10);
    }
  }
  delay(20);
}
