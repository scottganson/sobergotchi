// Finds which pins the EN05 board's buttons (and the reed switch) are wired to.
// Upload it, then press each button once, slowly, left to right. Each press
// shows a line on the screen such as "D1 low" (button pulls the pin to ground)
// or "D2 high" (button pulls it up). Also printed to Serial Monitor at 115200.
#include <GxEPD2_BW.h>
#include <SPI.h>

#define PIN_EPD_CS    7
#define PIN_EPD_DC    35
#define PIN_EPD_RST   30
#define PIN_EPD_BUSY  3
#define PIN_EPD_SCK   8
#define PIN_EPD_MOSI  10
#define PIN_EPD_POWER 6
#define PIN_NO_MISO   19

GxEPD2_BW<GxEPD2_213_GDEY0213B74, GxEPD2_213_GDEY0213B74::HEIGHT> epd(GxEPD2_213_GDEY0213B74(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

// Every pin not used by the display. Arduino number and label.
struct Cand { uint8_t pin; const char* name; };
const Cand CANDS[] = {
  {0, "D0"}, {1, "D1"}, {2, "D2"}, {4, "D4"}, {5, "D5"}, {9, "D9"},
  {31, "D12"}, {32, "D13"}, {33, "D14"}, {34, "D15"}, {36, "D17"}, {37, "D18"}, {38, "D19"},
};
const int N = sizeof(CANDS) / sizeof(CANDS[0]);
uint8_t base[N];  // bit0 = reading with pull-up, bit1 = reading with pull-down

const int LOGN = 10;
char logLines[LOGN][24];
int logCount = 0;
int events = 0;

uint8_t sample(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(200);
  uint8_t pu = digitalRead(pin);
  pinMode(pin, INPUT_PULLDOWN);
  delayMicroseconds(200);
  uint8_t pd = digitalRead(pin);
  return pu | (pd << 1);
}

void draw(bool full) {
  if (full) epd.setFullWindow();
  else epd.setPartialWindow(0, 0, epd.width(), epd.height());
  epd.firstPage();
  do {
    epd.fillScreen(GxEPD_WHITE);
    epd.fillRect(0, 0, epd.width(), 22, GxEPD_BLACK);
    epd.setTextColor(GxEPD_WHITE);
    epd.setTextSize(1);
    epd.setCursor(4, 4);
    epd.print("Button finder");
    epd.setCursor(4, 13);
    epd.print("press each button");
    epd.setTextColor(GxEPD_BLACK);
    epd.setTextSize(2);
    for (int i = 0; i < logCount; i++) {
      epd.setCursor(4, 30 + i * 21);
      epd.print(logLines[i]);
    }
    if (logCount == 0) {
      epd.setTextSize(1);
      epd.setCursor(4, 34);
      epd.print("waiting...");
    }
  } while (epd.nextPage());
}

void addLog(const char* s) {
  if (logCount == LOGN) {
    for (int i = 1; i < LOGN; i++) strcpy(logLines[i - 1], logLines[i]);
    logCount--;
  }
  strncpy(logLines[logCount], s, 23);
  logLines[logCount][23] = 0;
  logCount++;
}

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) delay(10);
  Serial.println("Button finder");
  pinMode(PIN_EPD_POWER, OUTPUT);
  digitalWrite(PIN_EPD_POWER, HIGH);
  delay(50);
  SPI.setPins(PIN_NO_MISO, PIN_EPD_SCK, PIN_EPD_MOSI);
  epd.init(0, true, 2, false);
  epd.setRotation(0);
  for (int i = 0; i < N; i++) base[i] = sample(CANDS[i].pin);
  Serial.print("idle states (pull-up/pull-down):");
  for (int i = 0; i < N; i++) {
    Serial.print(" ");
    Serial.print(CANDS[i].name);
    Serial.print("=");
    Serial.print(base[i] & 1);
    Serial.print("/");
    Serial.print(base[i] >> 1);
  }
  Serial.println();
  draw(true);
}

void loop() {
  for (int i = 0; i < N; i++) {
    uint8_t s = sample(CANDS[i].pin);
    if (s == base[i]) continue;
    delay(30);  // debounce: confirm
    s = sample(CANDS[i].pin);
    if (s == base[i]) continue;
    // pull-up reading went low = button to ground; pull-down reading went high = button to 3V3
    const char* how = (!(s & 1) && (base[i] & 1)) ? "low" : ((s & 2) && !(base[i] & 2)) ? "high" : "changed";
    char line[24];
    events++;
    snprintf(line, sizeof line, "%d. %s %s", events, CANDS[i].name, how);
    Serial.println(line);
    addLog(line);
    draw(false);
    uint32_t t0 = millis();
    while (sample(CANDS[i].pin) != base[i] && millis() - t0 < 5000) delay(10);  // wait for release
  }
  delay(10);
}
