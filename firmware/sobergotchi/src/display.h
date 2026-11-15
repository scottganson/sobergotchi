// The e-paper panel (GDEY0213B74 through GxEPD2), portrait 122 x 250.
#pragma once
#include <Arduino.h>
#include "ui/ui_types.h"

namespace display {

void begin();
// Draw a screen. full = slow flashing refresh that clears ghosting; otherwise a fast partial refresh.
// A full refresh is forced every FULL_REFRESH_EVERY partial ones.
void show(uint8_t screenId, UiData& data, bool full);
void sleep();  // panel into deep sleep (keeps the image, ~1 µA)
// Bring-up check: reset the panel, draw a screen with a label along the bottom, log whether the panel answered.
bool probe(const char* label, uint8_t screenId, UiData& data);

}  // namespace display
