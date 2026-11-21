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
  uint32_t setupDone;          // 0 until Pip's first-time setup is finished (fresh flash or factory reset); then the
                               // day number it was finished (Your path counts from it). 1 = finished before v1.34.
};

struct Friend {
  uint32_t uid;
  int32_t soberDay;
  char name[NAME_LEN + 1];
  char phone[PHONE_LEN + 1];
  uint16_t meetings;
  uint8_t story[3];            // their three story lines (Share your story), 1-based; 0 = not shared
};

// Your path: what has been celebrated and what was picked (kept in its own file, /path.bin).
struct Path {
  uint32_t magic;
  uint16_t seen;               // bit per Your path item: its "Something new!" has been shown
  uint8_t word;                // Your word: 0 not chosen yet, else 1-based
  uint8_t checkPick;           // daily check-in: what was picked on checkDay (0-based)
  int32_t checkDay;            // day number of the last check-in (0 = never)
  uint16_t thanks;             // thank-yous in the jar, all time
  uint16_t thankBits;          // what went in the jar on thankDay
  int32_t thankDay;
  uint8_t story[3];            // your three story lines, 1-based; 0 = not picked
  uint8_t leafBest;            // Leaf catch best score
  uint16_t tilesBest;          // Tree tiles fewest moves (0 = never solved)
  uint16_t tilesDone;          // times solved
};

namespace store {

extern Settings settings;
extern Friend friends[MAX_FRIENDS];
extern int friendCount;
extern Path path;

void begin();            // mount flash, load or create settings and friends
void saveSettings();
void saveFriends();
void savePath();
// Adds or updates a friend. Returns true if this is a new friend.
bool upsertFriend(const Friend& f);
void deleteFriend(int index);
void clearFriends();
// Forgets the name, phone, sobriety date and every friend, and gives the device a new ID. Keeps the date.
// Pip's first-time setup runs again afterwards.
void factoryReset();

}  // namespace store
