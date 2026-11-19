#include "store.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <stddef.h>

#include "datemath.h"

using namespace Adafruit_LittleFS_Namespace;

namespace store {

static const uint32_t MAGIC = 0x534F4252;  // "SOBR"
static const uint16_t VERSION = 1;
static const char* SETTINGS_FILE = "/settings.bin";
static const char* FRIENDS_FILE = "/friends.bin";

Settings settings;
Friend friends[MAX_FRIENDS];
int friendCount = 0;

static void writeFile(const char* path, const void* buf, size_t len) {
  InternalFS.remove(path);
  File f(InternalFS);
  if (f.open(path, FILE_O_WRITE)) {
    f.write((const uint8_t*)buf, len);
    f.close();
  }
}

static uint32_t randomId() {
  uint32_t r = 0;
  for (int i = 0; i < 4; i++) {  // hardware random number generator
    NRF_RNG->EVENTS_VALRDY = 0;
    NRF_RNG->TASKS_START = 1;
    while (!NRF_RNG->EVENTS_VALRDY) {}
    r = (r << 8) | NRF_RNG->VALUE;
    NRF_RNG->TASKS_STOP = 1;
  }
  return r ? r : 1;
}

static void defaults(uint32_t clockSecs) {
  memset(&settings, 0, sizeof settings);
  settings.magic = MAGIC;
  settings.version = VERSION;
  strncpy(settings.name, DEFAULT_NAME, NAME_LEN);
  strncpy(settings.phone, DEFAULT_PHONE, PHONE_LEN);
  settings.soberDay = daysFromCivil(DEFAULT_SOBER_YEAR, DEFAULT_SOBER_MONTH, DEFAULT_SOBER_DAY);
  settings.clockSecs = clockSecs;
  settings.uid = randomId();
  settings.setupDone = 0;  // Pip asks for the name, sobriety date and phone
}

void begin() {
  InternalFS.begin();
  // Settings saved before v1.12 end just before setupDone: keep them and count setup as done.
  const size_t oldSize = offsetof(Settings, setupDone);
  memset(&settings, 0, sizeof settings);
  File sf(InternalFS);
  size_t got = 0;
  if (sf.open(SETTINGS_FILE, FILE_O_READ)) {
    const int r = sf.read((uint8_t*)&settings, sizeof settings);
    got = r > 0 ? (size_t)r : 0;
    sf.close();
  }
  if (got == oldSize && settings.magic == MAGIC && settings.version == VERSION) {
    settings.setupDone = 1;
    saveSettings();
  } else if (got != sizeof settings || settings.magic != MAGIC || settings.version != VERSION) {
    defaults(0);
    saveSettings();
  }
  File f(InternalFS);
  friendCount = 0;
  if (f.open(FRIENDS_FILE, FILE_O_READ)) {
    uint16_t n = 0;
    if (f.read((uint8_t*)&n, 2) == 2) {
      if (n > MAX_FRIENDS) n = MAX_FRIENDS;
      if (f.read((uint8_t*)friends, n * sizeof(Friend)) == (int)(n * sizeof(Friend))) friendCount = n;
    }
    f.close();
  }
}

void saveSettings() { writeFile(SETTINGS_FILE, &settings, sizeof settings); }

void saveFriends() {
  InternalFS.remove(FRIENDS_FILE);
  File f(InternalFS);
  if (f.open(FRIENDS_FILE, FILE_O_WRITE)) {
    uint16_t n = (uint16_t)friendCount;
    f.write((const uint8_t*)&n, 2);
    f.write((const uint8_t*)friends, friendCount * sizeof(Friend));
    f.close();
  }
}

bool upsertFriend(const Friend& nf) {
  for (int i = 0; i < friendCount; i++) {
    if (friends[i].uid == nf.uid) {
      uint16_t meetings = friends[i].meetings;
      friends[i] = nf;  // name, date and phone may have changed
      friends[i].meetings = meetings + 1;
      saveFriends();
      return false;
    }
  }
  if (friendCount >= MAX_FRIENDS) return false;
  friends[friendCount] = nf;
  friends[friendCount].meetings = 1;
  friendCount++;
  saveFriends();
  return true;
}

void deleteFriend(int index) {
  if (index < 0 || index >= friendCount) return;
  memmove(&friends[index], &friends[index + 1], (friendCount - index - 1) * sizeof(Friend));
  friendCount--;
  saveFriends();
}

void clearFriends() {
  friendCount = 0;
  saveFriends();
}

void factoryReset() {
  defaults(settings.clockSecs);
  saveSettings();
  clearFriends();
}

}  // namespace store
