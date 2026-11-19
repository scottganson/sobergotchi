// Friend card swap over Bluetooth Low Energy, started when the reed switch closes or from Friends > Add friend.
// Both devices broadcast their card and listen for the other one for a few seconds; the radio is off otherwise.
// No pairing or connection: the card fits in an advert plus its scan response.
#pragma once
#include <Arduino.h>
#include "store.h"

namespace exchange {

void begin();   // start the Bluetooth stack (radio stays off until run())

struct Result {
  bool found;
  Friend card;
  uint8_t myThrow, theirThrow;  // rock-paper-scissors: 1 rock, 2 paper, 3 scissors
};

// Add friend mode (Friends > Add friend or a reed tap), in steps so the app can keep the radio on for the whole
// window while it asks the question: start() turns the radio on, found() says whether another Sobergotchi has been
// heard, best() gives the closest one so far, finish() turns the radio off.
void start(const Settings& me, int8_t minRssi);
bool found();
Result best();
void finish();

}  // namespace exchange
