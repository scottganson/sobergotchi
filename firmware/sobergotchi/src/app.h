// The app: which screen is showing, what the buttons do, and the values screens display.
#pragma once
#include <Arduino.h>

namespace app {

void begin();   // after hardware, storage and display are up
void loop();
bool bringupStep(const char* label);  // draws the home screen with a label (bring-up diagnostics)    // one turn: sleep until something happens, then react

}  // namespace app
