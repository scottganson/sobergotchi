#!/usr/bin/env python3
"""Draw the 26 tree stages as pen-and-ink style 1-bit pixel art (122x114).

Every pixel is placed directly (no anti-aliasing, no grey dithering) so the
result is exactly what the e-paper shows. Usage:
    python3 tools/make_tree_ink.py OUT_DIR      # writes stage-01.png .. stage-26.png
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image, ImageDraw

W, H = 122, 114
GROUND = 94
LIGHT = (0.64, -0.77)  # unit vector pointing at the sun (upper right)


def ground_y(x):
    t = (x - W / 2) / (W / 2)
    return GROUND + 4.5 * t * t


class Canvas:
    def __init__(self):
        self.a = np.zeros((H, W), np.uint8)  # 1 = ink
        self.keepout = np.zeros((H, W), bool)

    def px(self, x, y, c=1):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < W and 0 <= y < H:
            self.a[y, x] = c

    def line(self, x0, y0, x1, y1, c=1):
        x0, y0, x1, y1 = (int(round(v)) for v in (x0, y0, x1, y1))
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        e = dx + dy
        while True:
            self.px(x0, y0, c)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * e
            if e2 >= dy:
                e += dy
                x0 += sx
            if e2 <= dx:
                e += dx
                y0 += sy

    def poly(self, pts, c=1):
        """Thin 1px polyline."""
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            self.line(x0, y0, x1, y1, c)

    def disc(self, cx, cy, r, c=1):
        x0, x1 = max(0, int(cx - r - 1)), min(W - 1, int(cx + r + 1))
        y0, y1 = max(0, int(cy - r - 1)), min(H - 1, int(cy + r + 1))
        if x1 < x0 or y1 < y0:
            return
        yy, xx = np.mgrid[y0:y1 + 1, x0:x1 + 1]
        m = (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r
        self.a[y0:y1 + 1, x0:x1 + 1][m] = c

    def taper(self, pts, w0, w1, c=1):
        """Stroke along a polyline whose width runs from w0 to w1."""
        n = len(pts)
        seg = [math.dist(pts[i], pts[i + 1]) for i in range(n - 1)]
        total = sum(seg) or 1
        run = 0
        thin = []
        for i in range(n - 1):
            (xa, ya), (xb, yb) = pts[i], pts[i + 1]
            steps = max(1, int(seg[i] * 3))
            for k in range(steps + 1):
                t = k / steps
                w = w0 + (w1 - w0) * (run + seg[i] * t) / total
                x, y = xa + (xb - xa) * t, ya + (yb - ya) * t
                if w <= 1.25:
                    thin.append((x, y))
                else:
                    self.disc(x, y, w / 2, c)
            run += seg[i]
        if len(thin) > 1:
            self.poly(thin[::3] + [thin[-1]], c)

    def fill(self, pts, c=1):
        im = Image.new("1", (W, H), 0)
        ImageDraw.Draw(im).polygon([(float(x), float(y)) for x, y in pts], fill=1, outline=1)
        self.a[np.array(im, bool)] = c

    def ellipse_mask(self, cx, cy, rx, ry):
        yy, xx = np.mgrid[0:H, 0:W]
        return ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 <= 1

    def image(self):
        return Image.fromarray(((1 - self.a) * 255).astype(np.uint8), "L").convert("1")


def bez(p0, p1, p2, p3, n=24):
    out = []
    for i in range(n + 1):
        t = i / n
        u = 1 - t
        out.append((u ** 3 * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t ** 3 * p3[0],
                    u ** 3 * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t ** 3 * p3[1]))
    return out


def arc_curve(p0, p1, bend, n=16):
    """Curve from p0 to p1 bowed sideways by `bend` pixels."""
    mx, my = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
    dx, dy = p1[0] - p0[0], p1[1] - p0[1]
    d = math.hypot(dx, dy) or 1
    cx, cy = mx - dy / d * bend * 1.33, my + dx / d * bend * 1.33
    return bez(p0, (cx, cy), (cx, cy), p1, n)


# ---------------------------------------------------------------- sky

def sun(c, cx=104, cy=15, rays=True):
    if rays:
        for i in range(16):
            a = math.radians(i * 22.5 + 11.25)
            r0, r1 = 9.5, (14.5 if i % 2 == 0 else 11.5)
            c.line(cx + r0 * math.cos(a), cy + r0 * math.sin(a), cx + r1 * math.cos(a), cy + r1 * math.sin(a))
    im = Image.new("1", (W, H), 0)
    d = ImageDraw.Draw(im)
    d.ellipse((cx - 6, cy - 6, cx + 6, cy + 6), outline=1)
    c.a[np.array(im, bool)] = 1


BIG = ([(5, 4), (12, 7), (21, 5), (27, 3)], [(-2, 33, 0), (4, 38, 2), (12, 30, 4)])
SMALL = ([(3, 2), (8, 4), (14, 3)], [(-3, 19, 0), (2, 24, 2)])
WIDE = ([(4, 3), (11, 6), (20, 8), (30, 6), (38, 4), (44, 2)], [(-3, 49, 0), (2, 54, 2), (10, 46, 4), (18, 40, 6)])


def cloud(c, x, y, shape, shade=False, clear=2):
    """Engraved cloud: fine scalloped top, ruled lines along the base.
    shape = (lobes [(dx, r)], rules [(dx0, dx1, dy)]); shade rules the body too, for rain clouds."""
    lobes, rules = shape
    mask = np.zeros((H, W), bool)
    yy, xx = np.mgrid[0:H, 0:W]
    for dx, r in lobes:
        mask |= ((xx - (x + dx)) ** 2 + (yy - y) ** 2 <= r * r) & (yy <= y)
    edge = mask & ~(np.roll(mask, 1, 0) & np.roll(mask, -1, 1) & np.roll(mask, 1, 1))
    edge[y, :] = False
    # clear whatever is behind (sun rays, other clouds), with a little margin
    lo = min(d0 for d0, _, _ in rules)
    hi = max(d1 for _, d1, _ in rules)
    deep = max(dy for _, _, dy in rules)
    behind = mask.copy()
    behind[max(0, y - 1):y + deep + clear, max(0, x + lo - clear):min(W, x + hi + clear + 1)] = True
    for _ in range(clear):
        behind = grown(behind.astype(np.uint8)).astype(bool)
    c.a[behind] = 0
    c.a[edge] = 1
    if shade:
        inner = mask & np.roll(mask, 1, 1) & np.roll(mask, -1, 1) & np.roll(mask, 1, 0) & np.roll(mask, 2, 0)
        inner &= ((y - yy) % 2 == 0) & (yy < y)
        c.a[inner] = 1
    for dx0, dx1, dy in rules:
        c.line(x + dx0, y + dy, x + dx1, y + dy)


def rain(c, rng, top, gx, gy, ln, slant):
    y = top
    row = 0
    while y < GROUND + 6:
        x = -4 + (row % 2) * gx / 2
        while x < W + 6:
            px, py = x + rng.uniform(-1.5, 1.5), y + rng.uniform(-1.5, 1.5)
            c.line(px, py, px - slant, py + ln)
            x += gx
        y += gy
        row += 1


def snow(c, rng, top):
    y, row = top, 0
    while y < GROUND + 6:
        x = 2 + (row % 2) * 6
        while x < W:
            px, py = int(x + rng.uniform(-2, 2)), int(y + rng.uniform(-2, 2))
            if (row + int(x)) % 3 == 0:
                for dx, dy in ((0, 0), (-1, 0), (1, 0), (0, -1), (0, 1)):
                    c.px(px + dx, py + dy)
            else:
                c.px(px, py)
            x += 12
        y += 8
        row += 1


def wind(c, x0, x1, y, r=3, up=True):
    c.line(x0, y, x1, y)
    s = -1 if up else 1
    pts = []
    for i in range(15):
        a = math.radians(90 - i * 19)
        pts.append((x1 + r * math.cos(a), y + s * (r - r * math.sin(a))))
    c.poly(pts)


def bird(c, x, y, span=3):
    for i in range(-span, span + 1):
        c.px(x + i, y - (1 if abs(i) in (1, span) else 2 if abs(i) > 1 else 0))


def rainbow(c, cx=61, cy=97):
    for k, r in enumerate((62, 59, 56)):
        n = 0
        last = None
        for i in range(0, 1801):
            a = math.radians(180 + i / 10)
            p = (int(round(cx + r * math.cos(a))), int(round(cy + r * math.sin(a))))
            if p != last:
                last = p
                n += 1
                if k != 1 or n % 2 == 0:
                    c.px(*p)


def storm_clouds(c, shade):
    cloud(c, 74, 17, WIDE, shade)
    cloud(c, -6, 14, WIDE, shade)
    cloud(c, 36, 22, WIDE, shade)


def sky_clear(c, rng):
    sun(c)
    cloud(c, 12, 9, SMALL)


def sky_fair(c, rng):
    sun(c)
    cloud(c, 5, 15, BIG)
    cloud(c, 50, 7, SMALL)


def sky_morning(c, rng):
    sun(c, 18, 15)
    cloud(c, 82, 17, BIG)
    cloud(c, 48, 8, SMALL)


def sky_partly(c, rng):
    sun(c, 98, 14)
    cloud(c, 66, 24, WIDE)
    cloud(c, 6, 12, BIG)


def sky_birds(c, rng):
    sun(c)
    cloud(c, 8, 12, SMALL)
    bird(c, 38, 24)
    bird(c, 50, 17, 2)
    bird(c, 62, 27, 2)


def sky_overcast(c, rng):
    cloud(c, 70, 12, WIDE)
    cloud(c, -8, 11, WIDE)
    cloud(c, 30, 20, WIDE)
    cloud(c, 88, 27, BIG)
    cloud(c, 2, 29, SMALL)


def sky_windy(c, rng):
    sun(c)
    cloud(c, 6, 9, SMALL)
    wind(c, 4, 44, 26)
    wind(c, 22, 70, 34, 4, up=False)
    wind(c, 2, 30, 44, 3)
    wind(c, 78, 108, 42, 3)
    for x, y, a in ((52, 50, 20), (84, 30, -30), (100, 56, 35)):
        c.a[leaf_mask(x, y, a, 5, 0.5)] = 1


def sky_drizzle(c, rng):
    storm_clouds(c, False)
    rain(c, rng, 34, 15, 11, 3, 1)


def sky_rain(c, rng):
    storm_clouds(c, True)
    rain(c, rng, 33, 10, 8, 4, 2)


def sky_storm(c, rng):
    storm_clouds(c, True)
    rain(c, rng, 33, 10, 8, 4, 2)
    bolt = [(31, 29), (25, 41), (31, 41), (22, 58)]
    yy, xx = np.mgrid[0:H, 0:W]
    c.a[(xx > 16) & (xx < 38) & (yy > 28) & (yy < 62)] = 0
    for dx in (0, 1):
        c.poly([(x + dx, y) for x, y in bolt])


def sky_snow(c, rng):
    storm_clouds(c, False)
    snow(c, rng, 34)


def sky_fog(c, rng):
    sun(c, rays=False)
    cloud(c, 10, 12, BIG)
    for k, y in enumerate((52, 60, 68, 76, 84)):
        x = -rng.randint(0, 14) + (k % 2) * 9
        while x < W:
            ln = rng.randint(18, 40)
            for xx in range(max(0, x), min(W, x + ln)):
                if k % 2 == 0 or xx % 2 == 0:
                    c.px(xx, y)
            x += ln + rng.randint(6, 12)


def sky_rainbow(c, rng):
    rainbow(c)
    sun(c)
    cloud(c, 2, 20, WIDE)


SKIES = [  # order matters: the firmware picks by index (see skyFor in app.cpp)
    ("clear", sky_clear), ("fair", sky_fair), ("morning", sky_morning), ("partly", sky_partly),
    ("birds", sky_birds), ("overcast", sky_overcast), ("windy", sky_windy), ("drizzle", sky_drizzle),
    ("rain", sky_rain), ("storm", sky_storm), ("snow", sky_snow), ("fog", sky_fog), ("rainbow", sky_rainbow),
]


def draw_sky(i):
    c = Canvas()
    SKIES[i][1](c, random.Random(900 + i))
    return c


# ---------------------------------------------------------------- ground

def hills(c, rng):
    for x0, x1, top, base in ((-14, 62, 83, 97), (56, 140, 86, 98)):
        mid, half = (x0 + x1) / 2, (x1 - x0) / 2
        pts = [(x, base - (base - top) * math.cos((x - mid) / half * math.pi / 2)) for x in range(max(0, x0), min(W, x1 + 1))]
        pts = [(x, y) for x, y in pts if y < ground_y(x) - 1]
        for i, (x, y) in enumerate(pts):  # dotted far ridge: recedes behind the solid ground line
            if i % 2 == 0:
                c.px(x, y)


def ground(c, rng, soil=False, shadow=0):
    for x in range(W):
        gy = int(round(ground_y(x)))
        c.a[gy:, x] = 0
        c.a[gy, x] = 1
    # ruled lines under the surface, thinning with depth
    for k, dy in enumerate((3, 5, 8, 11, 14, 17)):
        keep = (0.62, 0.5, 0.4, 0.3, 0.22, 0.15)[k]
        x = rng.randint(-4, 4)
        while x < W:
            ln = rng.randint(3, 11)
            if rng.random() < keep:
                for xx in range(max(0, x), min(W, x + ln)):
                    yy = int(round(ground_y(xx))) + dy
                    if yy < H:
                        c.a[yy, xx] = 1
            x += ln + rng.randint(2, 6)
    if shadow:  # ruled shadow on the side away from the sun
        for dy, x0, x1 in ((2, 61 - shadow, 58), (4, 61 - shadow * 0.8, 52), (6, 61 - shadow * 0.5, 47)):
            for xx in range(int(x0), int(x1)):
                c.a[int(round(ground_y(xx))) + dy, xx] = 1


def grass(c, rng, skip=()):
    for x in (7, 21, 36, 49, 74, 88, 101, 115):
        x += rng.randint(-2, 2)
        if any(a <= x <= b for a, b in skip):
            continue
        gy = int(round(ground_y(x)))
        blades = rng.choice((((-2, 3), (0, 5), (2, 4)), ((-1, 4), (2, 3)), ((-2, 4), (1, 5), (3, 2))))
        for dx, h in blades:
            c.line(x, gy - 1, x + dx, gy - h)


def flower(c, x):
    gy = int(round(ground_y(x)))
    c.line(x, gy, x, gy - 5)
    for dx, dy in ((0, -8), (-1, -7), (1, -7), (0, -6)):
        c.px(x + dx, gy + dy)
    c.px(x + 1, gy - 3)


# ---------------------------------------------------------------- plant parts

def leaf(c, bx, by, ang, ln, fat=0.46, vein=True):
    """Almond leaf from its base (bx, by) pointing along ang (degrees, 0 = right, -90 = up)."""
    c.a[leaf_mask(bx, by, ang, ln, fat)] = 1
    if vein and ln >= 8:
        a = math.radians(ang)
        ux, uy = math.cos(a), math.sin(a)
        c.line(bx + ux * ln * 0.25, by + uy * ln * 0.25, bx + ux * ln * 0.72, by + uy * ln * 0.72, 0)


def apple(c, x, y, halo=True):
    if halo:
        c.disc(x, y, 5.2, 0)
        c.keepout |= c.ellipse_mask(x, y, 5.2, 5.2)
    c.disc(x, y, 3.3)
    c.px(x + 1, y - 2, 0)
    c.px(x + 2, y - 1, 0)
    c.line(x, y - 3, x + 1, y - 5)


def roots(c, x, y, depth, rng, n=3):
    for i in range(n):
        s = (i - (n - 1) / 2) / max(1, (n - 1) / 2) if n > 1 else 0.4
        ex, ey = x + s * depth * 0.7 + rng.uniform(-1, 1), y + depth * (1 - 0.35 * abs(s))
        c.poly(arc_curve((x, y), (ex, ey), -s * 2.2, 8))


def seed(c, cx, cy, rx, ry, ang):
    a = math.radians(ang)
    pts = []
    for i in range(40):
        t = i / 40 * 2 * math.pi
        px, py = rx * math.cos(t) * (1 + 0.18 * math.cos(t)), ry * math.sin(t)
        pts.append((cx + px * math.cos(a) - py * math.sin(a), cy + px * math.sin(a) + py * math.cos(a)))
    c.a[c.ellipse_mask(cx, cy, rx + 3.5, ry + 3)] = 0
    c.fill(pts)
    c.px(cx - rx * 0.35, cy - ry * 0.35, 0)
    c.px(cx - rx * 0.35 + 1, cy - ry * 0.35 - 1, 0)


# ---------------------------------------------------------------- sapling (months 1-11)

BRANCHES = [  # (height on stem, side, first month)
    (11, -1, 1), (20, 1, 3), (30, -1, 6), (40, 1, 9),
]


def sapling(c, month, rng):
    cx = W / 2
    gy = ground_y(cx)
    h = 28 + 3.6 * (month - 1)
    sway = 4.0
    stem = bez((cx, gy + 1), (cx + sway, gy - h * 0.35), (cx - sway, gy - h * 0.7), (cx + 0.6, gy - h), 60)

    def at(height):
        return min(stem, key=lambda p: abs((gy - p[1]) - height))

    c.taper(stem, 1.5 + month * 0.15, 1.0)
    tx, ty = stem[-1]
    leaf(c, tx, ty + 1, -80, 10, 0.44)
    leaf(c, tx - 0.5, ty + 4, -142, 6, 0.46, vein=False)
    for bi, (bh, side, first) in enumerate(BRANCHES):
        if month < first:
            continue
        nxt = BRANCHES[bi + 1][2] if bi + 1 < len(BRANCHES) else 12
        leaves = min(month, nxt - 1) - first + 1
        age = month - first
        bx, by = at(bh)
        ln = 7 + 4.5 * leaves + min(age, 6) * 0.5
        up = math.radians(38 + 4 * bi)
        ex, ey = bx + side * ln * math.cos(up), by - ln * math.sin(up)
        tw = arc_curve((bx, by), (ex, ey), side * ln * 0.1, 16)
        c.taper(tw, 1.5 if age > 3 else 1.0, 1.0)
        ta = tangent(tw, len(tw) - 1)
        leaf(c, ex, ey, ta, 12, 0.44)
        if leaves >= 2:
            px, py = tw[9]
            leaf(c, px, py, ta - side * 62, 10.5, 0.44)
        if leaves >= 3:
            px, py = tw[5]
            leaf(c, px, py, ta + side * 58, 10, 0.44)


# ---------------------------------------------------------------- tree (years)

def grown(a):
    """Ink grown by one pixel in every direction (used to keep shapes from touching)."""
    g = a.copy()
    g[1:, :] |= a[:-1, :]
    g[:-1, :] |= a[1:, :]
    g[:, 1:] |= a[:, :-1]
    g[:, :-1] |= a[:, 1:]
    return g


def leaf_mask(bx, by, ang, ln, fat):
    a = math.radians(ang)
    ux, uy = math.cos(a), math.sin(a)
    yy, xx = np.mgrid[0:H, 0:W]
    t = ((xx - bx) * ux + (yy - by) * uy) / ln
    s = np.abs(-(xx - bx) * uy + (yy - by) * ux)
    tc = np.clip(t, 0, 1)
    half = ln * fat * 0.5 * np.sin(np.pi * tc ** 0.75) ** 0.8
    return (t >= -0.05) & (t <= 1.02) & (s <= np.maximum(half, 0.45))


def small_leaf(c, bx, by, ang, ln=6.0, fat=0.5):
    """Place a leaf only if it stays clear of everything already drawn."""
    m = leaf_mask(bx, by, ang, ln, fat)
    yy, xx = np.mgrid[0:H, 0:W]
    far = (xx - bx) ** 2 + (yy - by) ** 2 > 2.6 ** 2
    if (m & far & grown(c.a).astype(bool)).any():
        return False
    c.a[m] = 1
    return True


def twig(c, pts, w0=1.0, w1=1.0):
    """Draw a branch, stopping short if it would run into something."""
    g = grown(c.a).astype(bool)
    keep = [pts[0]]
    for x, y in pts[1:]:
        xi, yi = int(round(x)), int(round(y))
        if not (0 <= xi < W and 0 <= yi < H):
            break
        if math.dist((x, y), pts[0]) > 3.5 and g[yi, xi]:
            break
        keep.append((x, y))
    if len(keep) > 3:
        c.taper(keep, w0, w1)
        return keep
    return None


def tangent(pts, i):
    i = max(1, min(len(pts) - 1, i))
    return math.degrees(math.atan2(pts[i][1] - pts[i - 1][1], pts[i][0] - pts[i - 1][0]))


def leaves_along(c, pts, start=0.3, gap=4.6, ln=6.0, side0=1):
    run, nxt, side = 0.0, None, side0
    total = sum(math.dist(pts[i], pts[i + 1]) for i in range(len(pts) - 1))
    nxt = total * start
    for i in range(1, len(pts)):
        run += math.dist(pts[i - 1], pts[i])
        if run >= nxt and run < total - 2:
            small_leaf(c, pts[i][0], pts[i][1], tangent(pts, i) + side * 52, ln)
            side = -side
            nxt += gap / 2
    small_leaf(c, pts[-1][0], pts[-1][1], tangent(pts, len(pts) - 1), ln + 0.5)


def tree(c, rng, scale, apples, base_x=W / 2):
    """Grow branches toward evenly spread points in the crown (space colonisation),
    thicken them by how much they carry, then set a leaf wherever there is room."""
    gy = ground_y(base_x)
    k = scale
    centre = (base_x, gy - 53 * k)
    RX, RY = 44 * k, 30 * k
    # points the branches reach for
    targets = []
    tries = 0
    while tries < 4000:
        tries += 1
        a, r = rng.uniform(0, 2 * math.pi), math.sqrt(rng.uniform(0.02, 1))
        p = (centre[0] + RX * r * math.cos(a), centre[1] + RY * r * math.sin(a) * (1.0 if math.sin(a) < 0 else 0.8))
        if p[1] < 4 or not (3 < p[0] < W - 4):
            continue
        if all(math.dist(p, q) >= 5.6 for q in targets):
            targets.append(p)
    # trunk
    nodes = [(base_x - 0.5, gy + 1.0)]
    parent = [-1]
    trunk_top = gy - 22 * k
    y = gy + 1.0
    while y > trunk_top:
        y -= 2
        f = (gy - y) / (22 * k)
        nodes.append((base_x + 2.2 * k * math.sin(f * math.pi * 1.5) * 0.8, y))
        parent.append(len(nodes) - 2)
    n_trunk = len(nodes)
    STEP, REACH, KILL = 2.0, 26 * k + 8, 2.6
    alive = list(targets)
    for _ in range(200):
        pull = {}
        for t in alive:
            best, bd = None, REACH
            for i in range(n_trunk - 1, len(nodes)):
                d = math.dist(t, nodes[i])
                if d < bd:
                    best, bd = i, d
            if best is not None:
                v = pull.setdefault(best, [0.0, 0.0])
                v[0] += (t[0] - nodes[best][0]) / bd
                v[1] += (t[1] - nodes[best][1]) / bd
        if not pull:
            break
        for i, (vx, vy) in pull.items():
            d = math.hypot(vx, vy)
            if d < 1e-6:
                continue
            q = (nodes[i][0] + vx / d * STEP, nodes[i][1] + vy / d * STEP - 0.12)
            if any(math.dist(q, n) < 0.9 for n in nodes[n_trunk - 1:]):
                continue
            nodes.append(q)
            parent.append(i)
        alive = [t for t in alive if all(math.dist(t, n) > KILL for n in nodes[n_trunk:])]
    kids = [[] for _ in nodes]
    for i, p in enumerate(parent):
        if p >= 0:
            kids[p].append(i)
    # smooth the kinks out of the branches
    for _ in range(5):
        new = list(nodes)
        for i in range(n_trunk, len(nodes)):
            if len(kids[i]) == 1:
                pa, ch = nodes[parent[i]], nodes[kids[i][0]]
                new[i] = ((pa[0] + ch[0] + 2 * nodes[i][0]) / 4, (pa[1] + ch[1] + 2 * nodes[i][1]) / 4)
        nodes = new
    # thickness: each branch carries the sum of what it feeds
    wt = [0.0] * len(nodes)
    for i in range(len(nodes) - 1, -1, -1):
        wt[i] = 1.0 if not kids[i] else sum(wt[j] ** 2.4 for j in kids[i]) ** (1 / 2.4)
    top_w = wt[n_trunk - 1]
    lim = 3.6 * k

    def width(i):
        if i < n_trunk:
            f = i / (n_trunk - 1)
            return 8.2 * k * (1 - f) + (lim + 0.4) * f
        return max(1.0, lim * (wt[i] / top_w) ** 1.5)

    for s_, reach in ((-1, 6.5), (1, 5.5)):  # trunk widens gently into the ground
        c.fill([(base_x + s_ * 2.5 * k, gy - 9 * k), (base_x + s_ * 3.6 * k, gy - 3 * k), (base_x + s_ * reach * k, gy + 1), (base_x, gy + 1)])
    order = sorted(range(1, len(nodes)), key=lambda i: -width(i))
    for i in order:
        c.taper([nodes[parent[i]], nodes[i]], width(parent[i]) if parent[i] >= n_trunk or i < n_trunk else width(i), width(i))
    # leaves: tips first, then alternating along the fine twigs
    def chain_angle(i):
        p = parent[i]
        pp = parent[p] if parent[p] >= 0 else p
        return math.degrees(math.atan2(nodes[i][1] - nodes[pp][1], nodes[i][0] - nodes[pp][0]))

    tips = [i for i in range(n_trunk, len(nodes)) if not kids[i]]
    for i in tips:
        small_leaf(c, nodes[i][0], nodes[i][1], chain_angle(i), 6.5) or small_leaf(c, nodes[i][0], nodes[i][1], chain_angle(i), 5.0)
    for i in range(n_trunk, len(nodes)):
        if kids[i] and width(i) < 1.9:
            depth = 0
            j = i
            while j >= n_trunk:
                depth += 1
                j = parent[j]
            side = 1 if depth % 2 else -1
            small_leaf(c, nodes[i][0], nodes[i][1], chain_angle(i) + side * 50, 5.6) or \
                small_leaf(c, nodes[i][0], nodes[i][1], chain_angle(i) - side * 50, 5.6)
    # apples hang in the crown on short stalks
    spots = [(-25, 14), (22, 16), (-3, 6), (34, 1), (-36, -1)][:apples]
    for dx, dy in spots:
        apple(c, centre[0] + dx * k, centre[1] + dy * k)


# ---------------------------------------------------------------- stages

def draw(stage, tree_seed=7100):
    rng = random.Random(7100 + stage)
    back = Canvas()
    hills(back, random.Random(5))
    cx = W / 2
    gy = ground_y(cx)
    shadow = 0 if stage < 16 else (16 + 3 * (stage - 16) if stage < 20 else 30)
    ground(back, random.Random(11), shadow=shadow)
    skip = [(cx - 9, cx + 9)]
    if stage >= 21:
        skip += [(66, 120)]
    grass(back, random.Random(3), skip)
    if stage >= 8:
        for x in (13, 109, 31, 92)[:min(4, 1 + (stage - 8) // 3)]:
            if stage < 21 or x == 13:
                flower(back, x)
    c = Canvas()

    if stage == 0:
        seed(c, cx, gy + 9, 4.6, 2.8, -16)
    elif stage == 1:
        seed(c, cx, gy + 9, 5.6, 3.6, -14)
        c.poly(arc_curve((cx + 4, gy + 11), (cx + 6, gy + 17), 1.5, 6))
    elif stage == 2:  # hook breaking the soil
        roots(c, cx - 2, gy + 3, 11, rng, 3)
        c.taper(bez((cx - 2, gy + 4), (cx - 3.5, gy - 11), (cx + 5, gy - 13), (cx + 6, gy - 6), 20), 1.8, 1.3)
        leaf(c, cx + 6, gy - 7, 84, 6, 0.5, vein=False)
    elif stage == 3:  # starting to stand
        roots(c, cx, gy + 3, 13, rng, 3)
        c.taper(bez((cx, gy + 3), (cx - 1.5, gy - 11), (cx + 2, gy - 19), (cx + 7, gy - 21), 20), 1.8, 1.2)
        leaf(c, cx + 6.5, gy - 21, -58, 7, 0.45, vein=False)
        leaf(c, cx + 6.5, gy - 21, 12, 7, 0.45, vein=False)
    elif stage == 4:  # standing, seed leaves open
        roots(c, cx, gy + 3, 14, rng, 4)
        c.taper(bez((cx, gy + 3), (cx + 2, gy - 9), (cx - 1.8, gy - 18), (cx, gy - 27), 20), 1.9, 1.2)
        leaf(c, cx, gy - 26.5, -143, 10, 0.5)
        leaf(c, cx, gy - 26.5, -37, 10, 0.5)
    elif stage <= 15:
        sapling(c, stage - 4, rng)
    else:
        years = stage - 15
        scale = (0.62, 0.72, 0.8, 0.88)[years - 1] if years <= 4 else 1.0
        tree(c, random.Random(tree_seed), scale, min(years, 5))
        for i in range(max(0, years - 5)):
            x = (80, 98, 112, 89, 71)[i]
            apple(c, x, ground_y(x) - 3, halo=False)
    # keep a clear margin of paper between the plant and whatever is behind it
    m = c.a.astype(bool)
    halo = grown(grown(c.a)).astype(bool)
    if stage >= 5:  # above ground only: the ground line runs right up to the trunk
        for x in range(W):
            halo[int(round(ground_y(x))):, x] = False
    if stage >= 16:
        halo[:int(gy) - 2, :] |= c.keepout[:int(gy) - 2, :]
    scenery = grown(back.a).astype(bool)
    back.a[halo] = 0
    back.a[m] = 1
    halo |= scenery
    # mask: where the sky must not show (around the plant, and everything below the ground line)
    mask = Canvas()
    mask.a[halo] = 1
    for x in range(W):
        mask.a[int(round(ground_y(x))):, x] = 1
    return back, mask


def compose(sky_c, plant, mask):
    out = Canvas()
    out.a = (sky_c.a & (1 - mask.a)) | plant.a
    return out.image()


def main():
    """No argument: write the firmware art into art/sky, art/tree and art/treemask.
    With a folder: also write composed previews there."""
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    skies = [draw_sky(i) for i in range(len(SKIES))]
    stages = [draw(i) for i in range(26)]
    for d in ("sky", "tree", "treemask"):
        os.makedirs(os.path.join(root, "art", d), exist_ok=True)
    for i, c in enumerate(skies):
        c.image().save(os.path.join(root, "art", "sky", f"sky-{i:02d}-{SKIES[i][0]}.png"))
    for i, (plant, mask) in enumerate(stages):
        plant.image().save(os.path.join(root, "art", "tree", f"stage-{i + 1:02d}.png"))
        mask.image().save(os.path.join(root, "art", "treemask", f"stage-{i + 1:02d}.png"))
    if len(sys.argv) > 1:
        out = sys.argv[1]
        os.makedirs(out, exist_ok=True)
        pad = 4

        def sheet(pairs, cols, name):
            sh = Image.new("1", (cols * (W + pad), ((len(pairs) + cols - 1) // cols) * (H + pad)), 1)
            for k, (sk, st) in enumerate(pairs):
                sh.paste(compose(skies[sk], *stages[st]), ((k % cols) * (W + pad), (k // cols) * (H + pad)))
            sh.save(os.path.join(out, name))

        sheet([(1, st) for st in range(26)], 7, "stages.png")
        sheet([(sk, 12) for sk in range(len(SKIES))], 5, "skies-sapling.png")
        sheet([(sk, 20) for sk in range(len(SKIES))], 5, "skies-tree.png")


if __name__ == "__main__":
    main()
