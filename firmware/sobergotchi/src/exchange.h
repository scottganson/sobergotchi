// Friend card swap over Bluetooth Low Energy, started when the reed switch closes.
// Both devices broadcast their card and listen for the other one for a few seconds.
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

Result run(const Settings& me);

}  // namespace exchange
