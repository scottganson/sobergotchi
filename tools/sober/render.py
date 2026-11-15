"""Preview renderer: draws compiled screens exactly the way the firmware does.

Keep this file in step with firmware/sobergotchi/src/ui/renderer.cpp.
"""
import datetime
import re

from PIL import Image

from .sml import (T_GROUP, T_TEXT, T_WRAP, T_IMAGE, T_BATTERY, T_DOTS, T_RULE, T_LIST,
                  F_INVERT, F_DOTTED, F_VERTICAL, FILL_LEVEL)
from .art import BAYER4
from .fonts import to_device

MONTHS = ["January", "February", "March", "April", "May", "June", "July", "August",
          "September", "October", "November", "December"]
DAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]


def format_date(d, fmt):
    """Same tokens as the firmware: dddd ddd MMMM MMM MM M dd d yyyy yy."""
    fmt = fmt or "MMM d, yyyy"
    out, i = [], 0
    tokens = ["dddd", "ddd", "MMMM", "MMM", "MM", "M", "dd", "d", "yyyy", "yy"]
    while i < len(fmt):
        for t in tokens:
            if fmt.startswith(t, i):
                v = {"dddd": DAYS[d.weekday()], "ddd": DAYS[d.weekday()][:3], "MMMM": MONTHS[d.month - 1],
                     "MMM": MONTHS[d.month - 1][:3], "MM": f"{d.month:02d}", "M": str(d.month),
                     "dd": f"{d.day:02d}", "d": str(d.day), "yyyy": str(d.year), "yy": f"{d.year % 100:02d}"}[t]
                out.append(v)
                i += len(t)
                break
        else:
            out.append(fmt[i])
            i += 1
    return "".join(out)


def format_number(n, fmt):
    if fmt == ",":
        return f"{int(n):,}"
    return str(n)


class SampleData:
    """Binding source for the preview, fed from tools/preview_data.json."""

    def __init__(self, data):
        self.data = data
        self.item = {}
        self.pages = 1

    def value(self, key, fmt):
        if key.startswith("item."):
            v = self.item.get(key[5:], "")
        else:
            v = self.data.get(key, "")
        if isinstance(v, dict) and "date" in v:
            return format_date(datetime.date.fromisoformat(v["date"]), fmt)
        if isinstance(v, bool):
            return "1" if v else ""
        if isinstance(v, (int, float)):
            return format_number(v, fmt)
        return to_device(str(v))

    def list_count(self, name):
        return len(self.data.get(name, []))

    def set_item(self, name, i):
        self.item = self.data.get(name, [])[i]


_TPL = re.compile(r"\{([A-Za-z0-9_.]+)(?::([^}]*))?\}")


def expand(tpl, src):
    if tpl is None:
        return ""
    return _TPL.sub(lambda m: src.value(m.group(1), m.group(2)), tpl)


def truthy(cond, src):
    if not cond:
        return True
    neg = cond.startswith("!")
    v = src.value(cond.lstrip("!"), None)
    t = v not in ("", "0", "false")
    return not t if neg else t


class Canvas:
    def __init__(self, w=122, h=250):
        self.w, self.h = w, h
        self.img = Image.new("1", (w, h), 1)
        self.px = self.img.load()

    def pixel(self, x, y, ink):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[x, y] = 0 if ink else 1

    def hline(self, x, y, w, ink):
        for i in range(w):
            self.pixel(x + i, y, ink)

    def vline(self, x, y, h, ink):
        for i in range(h):
            self.pixel(x, y + i, ink)

    def fill_rect(self, x, y, w, h, ink):
        for j in range(h):
            self.hline(x, y + j, w, ink)

    # Adafruit_GFX algorithms, ported one to one.
    def circle_helper(self, x0, y0, r, corner, ink):
        f, ddx, ddy, x, y = 1 - r, 1, -2 * r, 0, r
        while x < y:
            if f >= 0:
                y -= 1
                ddy += 2
                f += ddy
            x += 1
            ddx += 2
            f += ddx
            if corner & 4:
                self.pixel(x0 + x, y0 + y, ink); self.pixel(x0 + y, y0 + x, ink)
            if corner & 2:
                self.pixel(x0 + x, y0 - y, ink); self.pixel(x0 + y, y0 - x, ink)
            if corner & 8:
                self.pixel(x0 - y, y0 + x, ink); self.pixel(x0 - x, y0 + y, ink)
            if corner & 1:
                self.pixel(x0 - y, y0 - x, ink); self.pixel(x0 - x, y0 - y, ink)

    def fill_circle_helper(self, x0, y0, r, corners, delta, ink):
        f, ddx, ddy, x, y = 1 - r, 1, -2 * r, 0, r
        px, py = x, y
        delta += 1
        while x < y:
            if f >= 0:
                y -= 1
                ddy += 2
                f += ddy
            x += 1
            ddx += 2
            f += ddx
            if x < y + 1:
                if corners & 1:
                    self.vline(x0 + x, y0 - y, 2 * y + delta, ink)
                if corners & 2:
                    self.vline(x0 - x, y0 - y, 2 * y + delta, ink)
            if y != py:
                if corners & 1:
                    self.vline(x0 + py, y0 - px, 2 * px + delta, ink)
                if corners & 2:
                    self.vline(x0 - py, y0 - px, 2 * px + delta, ink)
                py = y
            px = x

    def draw_circle(self, x0, y0, r, ink):
        f, ddx, ddy, x, y = 1 - r, 1, -2 * r, 0, r
        for dx, dy in ((0, r), (0, -r), (r, 0), (-r, 0)):
            self.pixel(x0 + dx, y0 + dy, ink)
        while x < y:
            if f >= 0:
                y -= 1
                ddy += 2
                f += ddy
            x += 1
            ddx += 2
            f += ddx
            for dx, dy in ((x, y), (-x, y), (x, -y), (-x, -y), (y, x), (-y, x), (y, -x), (-y, -x)):
                self.pixel(x0 + dx, y0 + dy, ink)

    def fill_circle(self, x0, y0, r, ink):
        self.vline(x0, y0 - r, 2 * r + 1, ink)
        self.fill_circle_helper(x0, y0, r, 3, 0, ink)

    def fill_round_rect(self, x, y, w, h, r, ink):
        r = min(r, min(w, h) // 2)
        if r <= 0:
            return self.fill_rect(x, y, w, h, ink)
        self.fill_rect(x + r, y, w - 2 * r, h, ink)
        self.fill_circle_helper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, ink)
        self.fill_circle_helper(x + r, y + r, r, 2, h - 2 * r - 1, ink)

    def draw_round_rect(self, x, y, w, h, r, ink):
        r = min(r, min(w, h) // 2)
        if r <= 0:
            self.hline(x, y, w, ink); self.hline(x, y + h - 1, w, ink)
            self.vline(x, y, h, ink); self.vline(x + w - 1, y, h, ink)
            return
        self.hline(x + r, y, w - 2 * r, ink)
        self.hline(x + r, y + h - 1, w - 2 * r, ink)
        self.vline(x, y + r, h - 2 * r, ink)
        self.vline(x + w - 1, y + r, h - 2 * r, ink)
        self.circle_helper(x + r, y + r, r, 1, ink)
        self.circle_helper(x + w - r - 1, y + r, r, 2, ink)
        self.circle_helper(x + w - r - 1, y + h - r - 1, r, 4, ink)
        self.circle_helper(x + r, y + h - r - 1, r, 8, ink)

    def bitmap(self, x, y, art, ink):
        for j, row in enumerate(art.rows):
            for i, b in enumerate(row):
                if b:
                    self.pixel(x + i, y + j, ink)

    def glyph(self, x, y, g, ink):
        for j, row in enumerate(g.bits):
            for i, b in enumerate(row):
                if b:
                    self.pixel(x + g.x_offset + i, y + g.y_offset + j, ink)

    def text(self, x, baseline, s, font, ink):
        for ch in s:
            g = font.glyph(ch)
            self.glyph(x, baseline, g, ink)
            x += g.x_advance


def in_round_rect(px, py, x, y, w, h, r):
    r = min(r, min(w, h) // 2)
    if r <= 0:
        return True
    cx = x + r if px < x + r else (x + w - r - 1 if px > x + w - r - 1 else px)
    cy = y + r if py < y + r else (y + h - r - 1 if py > y + h - r - 1 else py)
    return (px - cx) ** 2 + (py - cy) ** 2 <= r * r


def fit_text(s, font, w):
    if font.text_width(s) <= w:
        return s
    while s and font.text_width(s + "..") > w:
        s = s[:-1]
    return s.rstrip() + ".."


def wrap_lines(s, font, w):
    lines, cur = [], ""
    for word in s.split(" "):
        if not word:
            continue
        cand = word if not cur else cur + " " + word
        if font.text_width(cand) <= w:
            cur = cand
            continue
        if cur:
            lines.append(cur)
        cur = word
        while font.text_width(cur) > w and len(cur) > 1:  # split very long words
            k = len(cur)
            while k > 1 and font.text_width(cur[:k]) > w:
                k -= 1
            lines.append(cur[:k])
            cur = cur[k:]
    if cur:
        lines.append(cur)
    return lines


class Renderer:
    def __init__(self, fonts, art_flat):
        self.fonts = fonts
        self.art = art_flat

    def draw(self, recs, src, canvas=None):
        c = canvas or Canvas()
        self.c, self.src = c, src
        self._range(recs, 0, len(recs), 0, False)
        return c

    def _range(self, recs, i, end, dy, inv):
        while i < end:
            r = recs[i]
            nxt = i + 1 + r["skip"]
            if truthy(r["cond"], self.src):
                self._node(recs, i, dy, inv)
            i = nxt

    def _decor(self, r, x, y, inv):
        c, w, h, rad = self.c, r["w"], r["h"], r["radius"]
        ink = not inv
        if r["fill"] == 1:
            c.fill_round_rect(x, y, w, h, rad, ink)
        elif r["fill"] == 2:
            c.fill_round_rect(x, y, w, h, rad, not ink)
        elif r["fill"] in FILL_LEVEL:
            lvl = FILL_LEVEL[r["fill"]]
            for py in range(y, y + h):
                for px in range(x, x + w):
                    if in_round_rect(px, py, x, y, w, h, rad):
                        c.pixel(px, py, ink if BAYER4[py & 3][px & 3] < lvl else not ink)
        for k in range(r["border"]):
            c.draw_round_rect(x + k, y + k, w - 2 * k, h - 2 * k, max(0, rad - k), ink)

    def _node(self, recs, i, dy, inv):
        r = recs[i]
        x, y, w, h = r["x"], r["y"] + dy, r["w"], r["h"]
        t, c = r["type"], self.c
        halign, valign = r["flags"] & 3, (r["flags"] >> 2) & 3
        self._decor(r, x, y, inv)
        inv = inv ^ bool(r["flags"] & F_INVERT)  # a node filled with ink draws its content in paper
        ink = not inv
        if t == T_TEXT:
            f = self.fonts[r["font"]]
            s = fit_text(expand(r["text"], self.src), f, w)
            tw = f.text_width(s)
            tx = x if halign == 0 else (x + (w - tw) // 2 if halign == 1 else x + w - tw)
            th = f.ascent + f.descent
            base = y + f.ascent if valign == 0 else (y + (h - th) // 2 + f.ascent if valign == 1 else y + h - f.descent)
            c.text(tx, base, s, f, ink)
        elif t == T_WRAP:
            f = self.fonts[r["font"]]
            lines = wrap_lines(expand(r["text"], self.src), f, w)
            lh = r["a"]
            per = max(1, (h - f.ascent - f.descent) // lh + 1)
            pages = max(1, (len(lines) + per - 1) // per)
            self.src.pages = pages
            page = min(int(expand(r.get("text2"), self.src) or 0), pages - 1)
            for k, line in enumerate(lines[page * per:(page + 1) * per]):
                tw = f.text_width(line)
                tx = x if halign == 0 else (x + (w - tw) // 2 if halign == 1 else x + w - tw)
                c.text(tx, y + f.ascent + k * lh, line, f, ink)
        elif t == T_IMAGE:
            idx = 0
            if r["text"]:
                try:
                    idx = int(expand(r["text"], self.src) or 0)
                except ValueError:
                    idx = 0
            idx = max(0, min(idx, r["b"] - 1))
            a = self.art[r["a"] + idx]
            ax = x if halign == 0 else (x + (w - a.width) // 2 if halign == 1 else x + w - a.width)
            ay = y if valign == 0 else (y + (h - a.height) // 2 if valign == 1 else y + h - a.height)
            c.bitmap(ax, ay, a, ink)
        elif t == T_BATTERY:
            pct = int(expand(r["text"], self.src) or 0)
            charging = truthy(r.get("text2"), self.src)
            bw, bh = 19, 10
            bx = x if halign == 0 else (x + (w - bw - 2) // 2 if halign == 1 else x + w - bw - 2)
            by = y + (h - bh) // 2
            c.draw_round_rect(bx, by, bw, bh, 2, ink)
            c.fill_rect(bx + bw, by + 3, 2, 4, ink)
            bars = 3 if pct > 66 else 2 if pct > 33 else 1 if pct > 10 else 0
            for k in range(bars):
                c.fill_rect(bx + 2 + k * 5, by + 2, 4, 6, ink)
            if charging:
                # lightning bolt to the left of the battery
                pts = [(bx - 3, by), (bx - 6, by + 5), (bx - 4, by + 5), (bx - 6, by + 10), (bx - 2, by + 4), (bx - 4, by + 4), (bx - 3, by)]
                for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
                    self._line(x0, y0, x1, y1, ink)
        elif t == T_DOTS:
            n = r["a"]
            cur = int(expand(r["text"], self.src) or 0)
            span = n * 5 + (n - 1) * 4
            sx = x + (w - span) // 2 + 2
            cy = y + h // 2
            for k in range(n):
                if k == cur:
                    c.fill_circle(sx + k * 9, cy, 2, ink)
                else:
                    c.draw_circle(sx + k * 9, cy, 2, ink)
        elif t == T_RULE:
            if r["flags"] & F_VERTICAL:
                cx = x + w // 2
                for k in range(h):
                    if not (r["flags"] & F_DOTTED) or (y + k) % 2 == 0:
                        c.pixel(cx, y + k, ink)
            else:
                cy = y + h // 2
                for k in range(w):
                    if not (r["flags"] & F_DOTTED) or (x + k) % 2 == 0:
                        c.pixel(x + k, cy, ink)
        elif t == T_LIST:
            name, rowh = r["text"], r["a"]
            count = self.src.list_count(name)
            sel = int(expand(r.get("text2"), self.src) or -1)
            rows = max(1, h // rowh)
            first = 0 if sel < rows else sel - rows + 1
            for k in range(first, min(count, first + rows)):
                self.src.set_item(name, k)
                ry = (k - first) * rowh
                if k == sel:
                    c.fill_rect(x, y + ry, w, rowh, ink)
                self._range(recs, i + 1, i + 1 + r["skip"], dy + ry, inv ^ (k == sel))
            return
        self._range(recs, i + 1, i + 1 + r["skip"], dy, inv)

    def _line(self, x0, y0, x1, y1, ink):
        # Bresenham, same as Adafruit_GFX::writeLine
        steep = abs(y1 - y0) > abs(x1 - x0)
        if steep:
            x0, y0, x1, y1 = y0, x0, y1, x1
        if x0 > x1:
            x0, x1, y0, y1 = x1, x0, y1, y0
        dx, dy = x1 - x0, abs(y1 - y0)
        err, ystep = dx // 2, 1 if y0 < y1 else -1
        while x0 <= x1:
            if steep:
                self.c.pixel(y0, x0, ink)
            else:
                self.c.pixel(x0, y0, ink)
            err -= dy
            if err < 0:
                y0 += ystep
                err += dx
            x0 += 1
