#include "renderer.h"

#include <GxEPD2.h>  // GxEPD_BLACK / GxEPD_WHITE

#include "../generated/fonts.h"
#include "../generated/art.h"
#include "../generated/screens.h"

namespace ui {

static const uint8_t BAYER4[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
static const uint8_t FILL_LEVEL[] = {0, 0, 0, 2, 4, 8, 12};  // inked cells of 16, by UiFill

const UiScreen& screen(uint8_t id) { return UI_SCREENS[id < SCREEN_COUNT ? id : 0]; }
uint8_t screenCount() { return SCREEN_COUNT; }
int screenByName(const char* id) {
  for (int i = 0; i < SCREEN_COUNT; i++)
    if (!strcmp(UI_SCREENS[i].id, id)) return i;
  return -1;
}

// ---------------------------------------------------------------------------
// templates
// ---------------------------------------------------------------------------
static UiData* g_data;
static Adafruit_GFX* g;

static void expand(const char* tpl, char* out, size_t n) {
  size_t len = 0;
  out[0] = 0;
  if (!tpl) return;
  while (*tpl && len + 1 < n) {
    if (*tpl == '{') {
      const char* end = strchr(tpl, '}');
      if (!end) break;
      char key[40], fmt[24];
      const char* colon = (const char*)memchr(tpl, ':', end - tpl);
      size_t klen = (colon ? colon : end) - tpl - 1;
      if (klen >= sizeof key) klen = sizeof key - 1;
      memcpy(key, tpl + 1, klen);
      key[klen] = 0;
      fmt[0] = 0;
      if (colon) {
        size_t flen = end - colon - 1;
        if (flen >= sizeof fmt) flen = sizeof fmt - 1;
        memcpy(fmt, colon + 1, flen);
        fmt[flen] = 0;
      }
      char val[160];
      val[0] = 0;
      g_data->value(key, colon ? fmt : nullptr, val, sizeof val);
      for (const char* v = val; *v && len + 1 < n; v++) out[len++] = *v;
      out[len] = 0;
      tpl = end + 1;
    } else {
      out[len++] = *tpl++;
      out[len] = 0;
    }
  }
}

static bool truthy(const char* cond) {
  if (!cond || !*cond) return true;
  bool neg = cond[0] == '!';
  char v[32];
  v[0] = 0;
  g_data->value(neg ? cond + 1 : cond, nullptr, v, sizeof v);
  bool t = v[0] && strcmp(v, "0") && strcmp(v, "false");
  return neg ? !t : t;
}

static int expandInt(const char* tpl, int def) {
  if (!tpl) return def;
  char v[16];
  expand(tpl, v, sizeof v);
  if (!v[0]) return def;
  return atoi(v);
}

// ---------------------------------------------------------------------------
// text
// ---------------------------------------------------------------------------
static const GFXglyph* glyphOf(const GFXfont* f, uint8_t c) {
  if (c < f->first || c > f->last) c = (uint8_t)'?';
  return &f->glyph[c - f->first];
}

int textWidth(uint8_t font, const char* s) {
  const GFXfont* f = UI_FONTS[font].font;
  int w = 0;
  for (; *s; s++) w += glyphOf(f, (uint8_t)*s)->xAdvance;
  return w;
}

static void drawText(int x, int baseline, const char* s, uint8_t font, uint16_t color) {
  const GFXfont* f = UI_FONTS[font].font;
  for (; *s; s++) {
    const GFXglyph* gl = glyphOf(f, (uint8_t)*s);
    const uint8_t* bm = f->bitmap + gl->bitmapOffset;
    uint8_t bits = 0, bit = 0;
    for (int yy = 0; yy < gl->height; yy++)
      for (int xx = 0; xx < gl->width; xx++) {
        if (!(bit++ & 7)) bits = *bm++;
        if (bits & 0x80) g->drawPixel(x + gl->xOffset + xx, baseline + gl->yOffset + yy, color);
        bits <<= 1;
      }
    x += gl->xAdvance;
  }
}

static void fitText(char* s, uint8_t font, int w) {
  if (textWidth(font, s) <= w) return;
  size_t len = strlen(s);
  char tmp[164];
  while (len > 0) {
    s[--len] = 0;
    snprintf(tmp, sizeof tmp, "%s..", s);
    if (textWidth(font, tmp) <= w) break;
  }
  while (len > 0 && s[len - 1] == ' ') s[--len] = 0;
  strcat(s, "..");
}

// Greedy word wrap. Calls fn(lineIndex, start, length) for each line. Returns the line count.
template <typename F>
static int wrapLines(const char* s, uint8_t font, int w, F fn) {
  const GFXfont* f = UI_FONTS[font].font;
  int lines = 0;
  const char* line = nullptr;  // start of current line
  int lineLen = 0, lineW = 0;
  const int spaceW = glyphOf(f, ' ')->xAdvance;
  while (*s) {
    while (*s == ' ') s++;
    if (!*s) break;
    const char* word = s;
    int wl = 0, ww = 0;
    while (s[wl] && s[wl] != ' ') ww += glyphOf(f, (uint8_t)s[wl++])->xAdvance;
    s += wl;
    if (line && lineW + spaceW + ww <= w) {
      lineLen = (int)(word + wl - line);
      lineW += spaceW + ww;
      continue;
    }
    if (line) fn(lines++, line, lineLen);
    line = word;
    lineLen = wl;
    lineW = ww;
    while (lineW > w && lineLen > 1) {  // split a very long word
      int k = lineLen, kw = lineW;
      while (k > 1 && kw > w) kw -= glyphOf(f, (uint8_t)line[--k])->xAdvance;
      fn(lines++, line, k);
      line += k;
      lineLen -= k;
      lineW -= kw;
    }
  }
  if (line) fn(lines++, line, lineLen);
  return lines;
}

// ---------------------------------------------------------------------------
// shapes
// ---------------------------------------------------------------------------
static bool inRoundRect(int px, int py, int x, int y, int w, int h, int r) {
  int m = (w < h ? w : h) / 2;
  if (r > m) r = m;
  if (r <= 0) return true;
  int cx = px < x + r ? x + r : (px > x + w - r - 1 ? x + w - r - 1 : px);
  int cy = py < y + r ? y + r : (py > y + h - r - 1 ? y + h - r - 1 : py);
  return (px - cx) * (px - cx) + (py - cy) * (py - cy) <= r * r;
}

static int clampRadius(int r, int w, int h) {
  int m = (w < h ? w : h) / 2;
  return r > m ? m : r;
}

static void decor(const UiNode& n, int x, int y, bool inv) {
  uint16_t ink = inv ? GxEPD_WHITE : GxEPD_BLACK, paper = inv ? GxEPD_BLACK : GxEPD_WHITE;
  int r = clampRadius(n.radius, n.w, n.h);
  if (n.fill == FILL_INK || n.fill == FILL_PAPER) {
    uint16_t c = n.fill == FILL_INK ? ink : paper;
    if (r > 0) g->fillRoundRect(x, y, n.w, n.h, r, c);
    else g->fillRect(x, y, n.w, n.h, c);
  } else if (n.fill >= FILL_G12) {
    uint8_t lvl = FILL_LEVEL[n.fill];
    for (int py = y; py < y + n.h; py++)
      for (int px = x; px < x + n.w; px++)
        if (inRoundRect(px, py, x, y, n.w, n.h, r)) g->drawPixel(px, py, BAYER4[py & 3][px & 3] < lvl ? ink : paper);
  }
  for (int k = 0; k < n.border; k++) {
    int rr = n.radius - k > 0 ? n.radius - k : 0;
    rr = clampRadius(rr, n.w - 2 * k, n.h - 2 * k);
    if (rr > 0) g->drawRoundRect(x + k, y + k, n.w - 2 * k, n.h - 2 * k, rr, ink);
    else g->drawRect(x + k, y + k, n.w - 2 * k, n.h - 2 * k, ink);
  }
}

// ---------------------------------------------------------------------------
// tree walk
// ---------------------------------------------------------------------------
static const UiNode* g_nodes;
static void drawRange(int i, int end, int dy, bool inv);

static void drawNode(int i, int dy, bool inv) {
  const UiNode& n = g_nodes[i];
  const int x = n.x, y = n.y + dy, w = n.w, h = n.h;
  const int ha = UI_HALIGN(n.flags), va = UI_VALIGN(n.flags);
  decor(n, x, y, inv);
  if (n.flags & UI_F_INVERT) inv = !inv;
  const uint16_t ink = inv ? GxEPD_WHITE : GxEPD_BLACK;
  char buf[164];

  switch (n.type) {
    case T_TEXT: {
      const UiFontInfo& fi = UI_FONTS[n.font];
      expand(n.text, buf, sizeof buf);
      fitText(buf, n.font, w);
      int tw = textWidth(n.font, buf);
      int tx = ha == 0 ? x : (ha == 1 ? x + (w - tw) / 2 : x + w - tw);
      int th = fi.ascent + fi.descent;
      int base = va == 0 ? y + fi.ascent : (va == 1 ? y + (h - th) / 2 + fi.ascent : y + h - fi.descent);
      drawText(tx, base, buf, n.font, ink);
      break;
    }
    case T_WRAP: {
      const UiFontInfo& fi = UI_FONTS[n.font];
      static char text[1200];
      expand(n.text, text, sizeof text);
      const int lh = n.a;
      int per = (h - fi.ascent - fi.descent) / lh + 1;
      if (per < 1) per = 1;
      int total = wrapLines(text, n.font, w, [](int, const char*, int) {});
      int pages = (total + per - 1) / per;
      if (pages < 1) pages = 1;
      g_data->wrapPages(pages);
      int page = expandInt(n.text2, 0);
      if (page > pages - 1) page = pages - 1;
      if (page < 0) page = 0;
      const int first = page * per, last = first + per;
      wrapLines(text, n.font, w, [&](int k, const char* s, int len) {
        if (k < first || k >= last) return;
        char line[160];
        if (len >= (int)sizeof line) len = sizeof line - 1;
        memcpy(line, s, len);
        line[len] = 0;
        int tw = textWidth(n.font, line);
        int tx = ha == 0 ? x : (ha == 1 ? x + (w - tw) / 2 : x + w - tw);
        drawText(tx, y + fi.ascent + (k - first) * lh, line, n.font, ink);
      });
      break;
    }
    case T_IMAGE: {
      int idx = expandInt(n.text, 0);
      if (idx < 0) idx = 0;
      if (idx > n.b - 1) idx = n.b - 1;
      const UiArt& a = UI_ART[n.a + idx];
      int ax = ha == 0 ? x : (ha == 1 ? x + (w - a.w) / 2 : x + w - a.w);
      int ay = va == 0 ? y : (va == 1 ? y + (h - a.h) / 2 : y + h - a.h);
      g->drawBitmap(ax, ay, a.data, a.w, a.h, ink);
      break;
    }
    case T_BATTERY: {
      int pct = expandInt(n.text, 0);
      bool charging = truthy(n.text2);
      const int bw = 19, bh = 10;
      int bx = ha == 0 ? x : (ha == 1 ? x + (w - bw - 2) / 2 : x + w - bw - 2);
      int by = y + (h - bh) / 2;
      g->drawRoundRect(bx, by, bw, bh, 2, ink);
      g->fillRect(bx + bw, by + 3, 2, 4, ink);
      int bars = pct > 66 ? 3 : pct > 33 ? 2 : pct > 10 ? 1 : 0;
      for (int k = 0; k < bars; k++) g->fillRect(bx + 2 + k * 5, by + 2, 4, 6, ink);
      if (charging) {
        const int8_t px[] = {-3, -6, -4, -6, -2, -4, -3}, py[] = {0, 5, 5, 10, 4, 4, 0};
        for (int k = 0; k < 6; k++) g->drawLine(bx + px[k], by + py[k], bx + px[k + 1], by + py[k + 1], ink);
      }
      break;
    }
    case T_DOTS: {
      int count = n.a, cur = expandInt(n.text, 0);
      int span = count * 5 + (count - 1) * 4;
      int sx = x + (w - span) / 2 + 2, cy = y + h / 2;
      for (int k = 0; k < count; k++) {
        if (k == cur) g->fillCircle(sx + k * 9, cy, 2, ink);
        else g->drawCircle(sx + k * 9, cy, 2, ink);
      }
      break;
    }
    case T_RULE: {
      bool dotted = n.flags & UI_F_DOTTED;
      if (n.flags & UI_F_VERTICAL) {
        int cx = x + w / 2;
        for (int k = 0; k < h; k++)
          if (!dotted || (y + k) % 2 == 0) g->drawPixel(cx, y + k, ink);
      } else {
        int cy = y + h / 2;
        for (int k = 0; k < w; k++)
          if (!dotted || (x + k) % 2 == 0) g->drawPixel(x + k, cy, ink);
      }
      break;
    }
    case T_LIST: {
      const int rowh = n.a;
      const int count = g_data->listCount(n.text);
      const int sel = expandInt(n.text2, -1);
      int rows = h / rowh;
      if (rows < 1) rows = 1;
      int first = sel < rows ? 0 : sel - rows + 1;
      for (int k = first; k < count && k < first + rows; k++) {
        g_data->setListItem(n.text, k);
        int ry = (k - first) * rowh;
        if (k == sel) g->fillRect(x, y + ry, w, rowh, ink);
        drawRange(i + 1, i + 1 + n.skip, dy + ry, inv != (k == sel));
      }
      return;  // children already drawn per row
    }
    default:
      break;
  }
  drawRange(i + 1, i + 1 + n.skip, dy, inv);
}

static void drawRange(int i, int end, int dy, bool inv) {
  while (i < end) {
    const UiNode& n = g_nodes[i];
    int next = i + 1 + n.skip;
    if (truthy(n.cond)) drawNode(i, dy, inv);
    i = next;
  }
}

void draw(Adafruit_GFX& gfx, uint8_t id, UiData& data) {
  const UiScreen& s = screen(id);
  g = &gfx;
  g_data = &data;
  g_nodes = s.nodes;
  drawRange(0, s.count, 0, false);
}

}  // namespace ui
