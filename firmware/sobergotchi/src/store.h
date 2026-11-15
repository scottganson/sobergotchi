// Settings and friends, kept in the chip's internal flash (LittleFS) so they survive
// resets, flat batteries and firmware updates.
#pragma once
#include <Arduino.h>
#include "config.h"

struct Settings {
  uint32_t magic;
  uint16_t version;
  char name[NAME_LEN + 1];
  char phone[PHONE_LEN + 1];   // digits only
  int32_t soberDay;            // day number (days since 1970-01-01)
  uint32_t clockSecs;          // last known local time, seconds since 1970-01-01
  uint32_t uid;                // random ID that identifies this device to friends
};

struct Friend {
  uint32_t uid;
  int32_t soberDay;
  char name[NAME_LEN + 1];
  char phone[PHONE_LEN + 1];
  uint16_t meetings;
};

namespace store {

extern Settings settings;
extern Friend friends[MAX_FRIENDS];
extern int friendCount;

void begin();            // mount flash, load or create settings and friends
void saveSettings();
void saveFriends();
// Adds or updates a friend. Returns true if this is a new friend.
bool upsertFriend(const Friend& f);
void deleteFriend(int index);
void clearFriends();

}  // namespace store
