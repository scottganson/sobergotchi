// Draws a compiled screen (see screens/*.sml) onto any Adafruit_GFX surface.
// Keep in step with tools/sober/render.py so the PNG previews match the device.
#pragma once
#include <Adafruit_GFX.h>
#include "ui_types.h"

namespace ui {

const UiScreen& screen(uint8_t id);
uint8_t screenCount();
int screenByName(const char* id);

// Draw screen 'id' using values from 'data'. Colours: ink = black, paper = white.
void draw(Adafruit_GFX& gfx, uint8_t id, UiData& data);

// Text helpers (also used by the app to measure)
int textWidth(uint8_t font, const char* s);

}  // namespace ui
