#include "exchange.h"

#include <bluefruit.h>

#include "config.h"
#include "datemath.h"

namespace exchange {

static const uint8_t KIND_CARD = 1, KIND_PHONE = 2, VERSION = 1;
static const int32_t DAY_2000 = 10957;  // daysFromCivil(2000, 1, 1)

struct __attribute__((packed)) CardAdv {
  uint16_t company;
  char magic[2];   // "SG"
  uint8_t kind;    // KIND_CARD
  uint8_t version;
  uint32_t uid;
  uint16_t soberDay2000;
  uint8_t rps;
  char name[NAME_LEN];  // not null terminated
};

struct __attribute__((packed)) PhoneRsp {
  uint16_t company;
  char magic[2];
  uint8_t kind;    // KIND_PHONE
  uint32_t uid;
  uint8_t bcd[8];  // up to 15 digits, 0xF ends
};

struct Candidate {
  bool used, gotCard, gotPhone;
  int8_t rssi;
  CardAdv card;
  PhoneRsp phone;
};

static Candidate cands[4];
static int8_t minRssi = ADD_FRIEND_MIN_RSSI;
static uint8_t myThrow = 1;

static void onScan(ble_gap_evt_adv_report_t* report) {
  uint8_t buf[32];
  int len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buf, sizeof buf);
  if (len >= 7 && buf[2] == 'S' && buf[3] == 'G' && report->rssi >= minRssi) {
    uint16_t company = buf[0] | (buf[1] << 8);
    uint32_t uid;
    uint8_t kind = buf[4];
    if (company == BLE_COMPANY_ID && (kind == KIND_CARD || kind == KIND_PHONE)) {
      memcpy(&uid, kind == KIND_CARD ? buf + 6 : buf + 5, 4);
      Candidate* c = nullptr;
      for (auto& k : cands)
        if (k.used && ((k.gotCard && k.card.uid == uid) || (k.gotPhone && k.phone.uid == uid))) c = &k;
      if (!c)
        for (auto& k : cands)
          if (!k.used) { c = &k; c->used = true; break; }
      if (c) {
        if (kind == KIND_CARD && len >= (int)sizeof(CardAdv)) { memcpy(&c->card, buf, sizeof(CardAdv)); c->gotCard = true; }
        if (kind == KIND_PHONE && len >= (int)sizeof(PhoneRsp)) { memcpy(&c->phone, buf, sizeof(PhoneRsp)); c->gotPhone = true; }
        if (report->rssi > c->rssi || c->rssi == 0) c->rssi = report->rssi;
      }
    }
  }
  Bluefruit.Scanner.resume();
}

void begin() {
  Bluefruit.autoConnLed(false);
  Bluefruit.begin(1, 1);
  Bluefruit.setName("Sobergotchi");
  Bluefruit.setTxPower(-8);  // short range on purpose
}

static uint8_t randomThrow() {
  uint8_t r = 0;
  sd_rand_application_vector_get(&r, 1);
  return 1 + r % 3;
}

void start(const Settings& me, int8_t rssiFloor) {
  memset(cands, 0, sizeof cands);
  minRssi = rssiFloor;
  myThrow = randomThrow();

  CardAdv card;
  memset(&card, 0, sizeof card);
  card.company = BLE_COMPANY_ID;
  card.magic[0] = 'S'; card.magic[1] = 'G';
  card.kind = KIND_CARD;
  card.version = VERSION;
  card.uid = me.uid;
  card.soberDay2000 = (uint16_t)(me.soberDay - DAY_2000);
  card.rps = myThrow;
  memcpy(card.name, me.name, strnlen(me.name, NAME_LEN));

  PhoneRsp phone;
  memset(&phone, 0, sizeof phone);
  phone.company = BLE_COMPANY_ID;
  phone.magic[0] = 'S'; phone.magic[1] = 'G';
  phone.kind = KIND_PHONE;
  phone.uid = me.uid;
  memset(phone.bcd, 0xFF, sizeof phone.bcd);
  for (int i = 0; i < PHONE_LEN && me.phone[i]; i++) {
    uint8_t d = me.phone[i] - '0';
    phone.bcd[i / 2] = (i & 1) ? ((phone.bcd[i / 2] & 0xF0) | d) : ((d << 4) | 0x0F);
  }

  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addManufacturerData(&card, sizeof card);
  Bluefruit.ScanResponse.addManufacturerData(&phone, sizeof phone);
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.setInterval(32, 48);  // 20-30 ms
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.start(0);

  Bluefruit.Scanner.setRxCallback(onScan);
  Bluefruit.Scanner.useActiveScan(true);  // ask for the scan response (phone number)
  Bluefruit.Scanner.setInterval(160, 160);
  Bluefruit.Scanner.restartOnDisconnect(false);
  Bluefruit.Scanner.start(0);
}

bool found() {
  for (auto& k : cands)
    if (k.used && k.gotCard) return true;
  return false;
}

void finish() {
  Bluefruit.Scanner.stop();
  Bluefruit.Advertising.stop();
}

Result best() {
  Result res;
  memset(&res, 0, sizeof res);
  res.myThrow = myThrow;
  Candidate* best = nullptr;
  for (auto& k : cands)
    if (k.used && k.gotCard && (!best || k.rssi > best->rssi)) best = &k;
  if (!best) return res;

  res.found = true;
  Friend& f = res.card;
  memset(&f, 0, sizeof f);
  f.uid = best->card.uid;
  f.soberDay = DAY_2000 + best->card.soberDay2000;
  memcpy(f.name, best->card.name, NAME_LEN);
  f.name[NAME_LEN] = 0;
  res.theirThrow = best->card.rps >= 1 && best->card.rps <= 3 ? best->card.rps : 1;
  if (best->gotPhone && best->phone.uid == f.uid) {
    int n = 0;
    for (int i = 0; i < PHONE_LEN; i++) {
      uint8_t b = best->phone.bcd[i / 2];
      uint8_t d = (i & 1) ? (b & 0x0F) : (b >> 4);
      if (d > 9) break;
      f.phone[n++] = '0' + d;
    }
    f.phone[n] = 0;
  }
  return res;
}

}  // namespace exchange
