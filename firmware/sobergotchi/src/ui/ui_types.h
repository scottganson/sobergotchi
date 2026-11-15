// Shared types between the generated screen tables and the renderer.
// Keep in step with tools/sober/sml.py.
#pragma once
#include <Arduino.h>

enum UiType : uint8_t { T_GROUP, T_TEXT, T_WRAP, T_IMAGE, T_BATTERY, T_DOTS, T_RULE, T_LIST };

// flags
#define UI_HALIGN(f)   ((f) & 3)          // 0 left, 1 center, 2 right
#define UI_VALIGN(f)   (((f) >> 2) & 3)   // 0 top, 1 middle, 2 bottom
#define UI_F_INVERT    (1 << 4)           // filled with ink: content drawn in paper colour
#define UI_F_DOTTED    (1 << 5)
#define UI_F_VERTICAL  (1 << 6)

// fill
enum UiFill : uint8_t { FILL_NONE, FILL_INK, FILL_PAPER, FILL_G12, FILL_G25, FILL_G50, FILL_G75 };

struct UiNode {
  uint8_t type, font, flags, fill, border, radius;
  int16_t x, y, w, h;
  int16_t a, b;            // type-specific numbers (line height, art index, count, row height)
  const char* text;        // template text, image index, list name...
  const char* text2;       // second template (wrap page, list selection, battery charging)
  const char* cond;        // "key" or "!key": node drawn only when true
  uint16_t skip;           // number of descendants that follow this node
};

struct UiScreen {
  const char* id;
  const UiNode* nodes;
  uint16_t count;
};

// The app provides the values that {placeholders} in screen files ask for.
class UiData {
 public:
  virtual void value(const char* key, const char* fmt, char* out, size_t n) = 0;
  virtual int listCount(const char* name) = 0;
  virtual void setListItem(const char* name, int index) = 0;
  virtual void wrapPages(int pages) { (void)pages; }
};
