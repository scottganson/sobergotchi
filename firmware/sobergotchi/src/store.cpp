#include "store.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <nrf_sdm.h>
#include <nrf_soc.h>
#include <stddef.h>

#include "datemath.h"

using namespace Adafruit_LittleFS_Namespace;

namespace store {

static const uint32_t MAGIC = 0x534F4252;  // "SOBR"
static const uint16_t VERSION = 1;
static const char* SETTINGS_FILE = "/settings.bin";
static const char* FRIENDS_FILE = "/friends.bin";
static const char* PATH_FILE = "/path.bin";
static const uint32_t PATH_MAGIC = 0x50415448;  // "PATH"

Settings settings;
Friend friends[MAX_FRIENDS];
int friendCount = 0;
Path path;

// Friends saved before v1.34 have no story: the same fields up to meetings.
struct FriendV1 {
  uint32_t uid;
  int32_t soberDay;
  char name[NAME_LEN + 1];
  char phone[PHONE_LEN + 1];
  uint16_t meetings;
};

static void pathDefaults() {
  memset(&path, 0, sizeof path);
  path.magic = PATH_MAGIC;
}

static void writeFile(const char* path, const void* buf, size_t len) {
  InternalFS.remove(path);
  File f(InternalFS);
  if (f.open(path, FILE_O_WRITE)) {
    f.write((const uint8_t*)buf, len);
    f.close();
  }
}

// A random device ID. Once Bluetooth is running the SoftDevice owns the random number generator, and touching
// NRF_RNG directly then faults or hangs (that is what froze the factory reset in v1.12 to v1.14), so ask it instead.
static uint32_t randomId() {
  uint32_t r = 0;
  uint8_t sdOn = 0;
  sd_softdevice_is_enabled(&sdOn);
  if (sdOn) {
    for (int tries = 0; tries < 50; tries++) {
      if (sd_rand_application_vector_get((uint8_t*)&r, sizeof r) == NRF_SUCCESS) break;
      delay(2);  // its pool refills in the background
    }
    if (!r) r = (uint32_t)micros() * 2654435761u ^ settings.uid;  // pool still empty: anything that differs from before
  } else {
    for (int i = 0; i < 4; i++) {  // hardware random number generator, before Bluetooth starts
      NRF_RNG->EVENTS_VALRDY = 0;
      NRF_RNG->TASKS_START = 1;
      while (!NRF_RNG->EVENTS_VALRDY) {}
      r = (r << 8) | NRF_RNG->VALUE;
      NRF_RNG->TASKS_STOP = 1;
    }
  }
  return r ? r : 1;
}

static void defaults(uint32_t clockSecs) {
  const uint32_t uid = randomId();
  memset(&settings, 0, sizeof settings);
  settings.magic = MAGIC;
  settings.version = VERSION;
  strncpy(settings.name, DEFAULT_NAME, NAME_LEN);
  strncpy(settings.phone, DEFAULT_PHONE, PHONE_LEN);
  settings.soberDay = daysFromCivil(DEFAULT_SOBER_YEAR, DEFAULT_SOBER_MONTH, DEFAULT_SOBER_DAY);
  settings.clockSecs = clockSecs;
  settings.uid = uid;
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
    const uint32_t size = f.size();
    uint16_t n = 0;
    if (f.read((uint8_t*)&n, 2) == 2) {
      if (n > MAX_FRIENDS) n = MAX_FRIENDS;
      memset(friends, 0, sizeof friends);
      if (size == 2 + n * sizeof(Friend)) {
        if (f.read((uint8_t*)friends, n * sizeof(Friend)) == (int)(n * sizeof(Friend))) friendCount = n;
      } else if (size == 2 + n * sizeof(FriendV1)) {  // before v1.34: no story; read one at a time and widen
        FriendV1 old;
        for (int i = 0; i < n && f.read((uint8_t*)&old, sizeof old) == (int)sizeof old; i++) {
          friends[i].uid = old.uid;
          friends[i].soberDay = old.soberDay;
          memcpy(friends[i].name, old.name, sizeof old.name);
          memcpy(friends[i].phone, old.phone, sizeof old.phone);
          friends[i].meetings = old.meetings;
          friendCount = i + 1;
        }
      }
    }
    f.close();
    if (friendCount && size != 2 + friendCount * sizeof(Friend)) saveFriends();  // save in the new layout
  }

  File pf(InternalFS);
  pathDefaults();
  bool pathOk = false;
  if (pf.open(PATH_FILE, FILE_O_READ)) {
    pathOk = pf.read((uint8_t*)&path, sizeof path) == (int)sizeof path && path.magic == PATH_MAGIC;
    pf.close();
  }
  if (!pathOk) pathDefaults();
}

void saveSettings() { writeFile(SETTINGS_FILE, &settings, sizeof settings); }

void savePath() { writeFile(PATH_FILE, &path, sizeof path); }

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
      uint8_t story[3];
      memcpy(story, friends[i].story, 3);
      friends[i] = nf;  // name, date, phone and story may have changed
      if (!nf.story[0]) memcpy(friends[i].story, story, 3);  // an older device sends no story: keep what we had
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
  pathDefaults();
  savePath();
}

}  // namespace store
