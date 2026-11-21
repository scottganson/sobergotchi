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
  int32_t metDay;              // day we last met (0 = before v1.34)
  uint16_t points;             // their points in pointsMonth, as of metDay
  uint16_t pointsMonth;        // months since 2000 (0 = unknown)
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
  char ownWord[OWN_LEN + 1];   // Your word, written by them (path.word == 8)
  char ownThank[OWN_LEN + 1];  // their own thank-you (Gratitude jar)
  // Year coins (Awards): bit per coin, its "A new coin!" has been shown (coins earned before setup count as shown)
  uint16_t coinsSeen;
  char engraving[COIN_COUNT][ENGRAVE_LEN + 1];  // the back of each coin; empty until engraved (then locked)
  // Points (Leaderboard): this month's points
  uint16_t points;
  uint16_t pointsMonth;        // months since 2000
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
