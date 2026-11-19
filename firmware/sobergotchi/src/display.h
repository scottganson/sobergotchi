// The e-paper panel (GDEY0213B74 through GxEPD2), portrait 122 x 250.
#pragma once
#include <Arduino.h>
#include "ui/ui_types.h"

namespace display {

void begin();
// Draw a screen. full = slow flashing refresh that clears ghosting; otherwise a fast partial refresh.
void show(uint8_t screenId, UiData& data, bool full);
void sleep();  // panel into deep sleep (keeps the image, ~1 µA)
bool probe(const char* label);  // bring-up check: full refresh with a label, true if the panel answered

}  // namespace display
