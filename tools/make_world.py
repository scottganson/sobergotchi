#!/usr/bin/env python3
"""Concept: the camera changes day by day. The years decide what exists (the house grows from five to ten years,
then a new building arrives every few years); each day picks a view: wide (the whole community), middle (the tree
and house) or close (inside a room).

    python3 tools/make_world.py OUT_DIR     # writes world-week-y8@3x.png and world-week-y15@3x.png (previews only)
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_tree_ink as T  # noqa: E402
import make_gnome as G  # noqa: E402

W, H = T.W, T.H
NEAR = 101          # ground line in the wide view (foreground)


def near_y(x):
    return NEAR + 2.5 * ((x - W / 2) / (W / 2)) ** 2


def far_y(x):
    """The far hill the newer buildings sit on: rises to the right."""
    return 80 - 9 * math.sin(max(0.0, min(1.0, (x - 50) / 72)) * math.pi * 0.5) + 2 * math.sin(x / 9)


# arrivals after ten years (my proposal): bridge 11, well 13, friend's cottage 15
BRIDGE, WELL, COTTAGE = 11, 13, 15


def wide_tree(c, bx, level, rng):
    """A small tree with the house in its crown, sized for the wide view. level 1-6 = years 5-10."""
    by = int(near_y(bx))
    # trunk with flared roots
    c.fill([(bx - 9, by), (bx + 9, by), (bx + 5, by - 4), (bx + 4, by - 24), (bx - 4, by - 24), (bx - 5, by - 4)])
    c.line(bx - 1, by - 3, bx - 2, by - 16, 0)
    c.px(bx + 2, by - 10, 0)
    # the house sits on the trunk
    hw = 10 + level                     # half width grows with the years
    floor = by - 26
    walls = 10
    house = T.Canvas()
    G.white(house, [(bx - hw - 1, floor), (bx + hw + 1, floor), (bx + hw, floor - walls), (bx - hw, floor - walls)])
    top = floor - walls
    if level >= 3:                      # an upper floor set off to the right
        ux0, ux1 = bx - hw + 5, bx + hw + 3
        G.white(house, [(ux0, top), (ux1, top), (ux1 - 1, top - 8), (ux0 + 1, top - 8)])
        top -= 8
        cx = (ux0 + ux1) // 2
        span = (ux1 - ux0) // 2 + 4
        house.line(ux0, top + 8, ux1, top + 8)
    else:
        cx, span = bx, hw + 4
    peak = top - (9 + level)
    house.fill([(cx - span - 1, top + 1), (cx + span + 1, top + 1), (cx - 2, peak), (cx - 3, peak)])
    for yy in range(top - 2, peak + 2, -3):  # shingles
        for xx in range(cx - span, cx + span):
            if (xx + yy) % 4 == 0 and house.a[yy, xx] and house.a[yy, xx - 1] and house.a[yy, xx + 1]:
                house.px(xx, yy, 0)
    house.fill([(cx + span // 2, top - 3), (cx + span // 2 + 3, top - 3), (cx + span // 2 + 4, peak + 2), (cx + span // 2 + 1, peak + 2)])
    # windows and door
    for wx in range(bx - hw + 3, bx + hw - 4, 7):
        G.frame(house, wx, floor - walls + 3, wx + 3, floor - walls + 6)
    G.white(house, [(bx + hw - 5, floor), (bx + hw - 5, floor - 6), (bx + hw - 2, floor - 6), (bx + hw - 2, floor)])
    house.fill([(bx - hw - 2, floor), (bx + hw + 2, floor), (bx + hw + 2, floor + 1), (bx - hw - 2, floor + 1)])
    # ladder down the right side
    lx = bx + hw - 3
    if lx < bx + 7:
        lx = bx + 7
    for yy in range(floor + 3, by, 3):
        house.line(lx, yy, lx + 3, yy)
    house.line(lx, floor + 1, lx, by - 1)
    house.line(lx + 3, floor + 1, lx + 3, by - 1)
    # crown of small leaves around the house
    hmask = T.grown(T.grown((house.a | house.keepout).astype(np.uint8))).astype(bool)
    taken = hmask.copy()
    ccx, ccy, rx, ry = bx, floor - 14, hw + 17, 24
    for _ in range(900):
        a, r = rng.uniform(0, 2 * math.pi), math.sqrt(rng.uniform(0.15, 1))
        x, y = ccx + rx * r * math.cos(a), ccy + ry * r * math.sin(a) * (0.85 if math.sin(a) > 0 else 1)
        if not (2 < x < W - 3 and 2 < y < by - 22):
            continue
        m = T.leaf_mask(x, y, math.degrees(a) + rng.uniform(-40, 40), 5, 0.5)
        if (m & taken).any():
            continue
        c.a[m] = 1
        taken |= T.grown(m.astype(np.uint8)).astype(bool)
    # a few branches reaching out from the trunk top
    for dx, dy in ((-hw - 8, -8), (hw + 9, -6), (-hw - 2, -20), (hw + 2, -22)):
        pts = [(bx + dx * t, floor - 1 + dy * t - 4 * math.sin(t * math.pi)) for t in np.linspace(0.3, 1, 8)]
        c.poly(pts)
    c.a[hmask] = 0
    c.a |= house.a
    c.keepout |= house.keepout
    return {"floor": floor, "left": bx - hw, "right": bx + hw, "ground": by}


BRIDGE_X0, BRIDGE_X1, BRIDGE_Y = 56, 88, 97


def deck_y(x):
    t = (x - BRIDGE_X0) / (BRIDGE_X1 - BRIDGE_X0)
    return BRIDGE_Y - 6 * math.sin(max(0, min(1, t)) * math.pi)


def bridge(c):
    """A stream winding down from the far hill, with an arched footbridge."""
    left = [(78, far_y(78)), (75, 86), (68, 94), (61, 103), (54, H)]
    right = [(84, far_y(84)), (83, 86), (79, 94), (74, 103), (70, H)]
    tmp = T.Canvas()
    tmp.fill(left + right[::-1])
    water = tmp.a.astype(bool)
    c.a[water] = 0
    c.keepout |= water
    c.poly(left)
    c.poly(right)
    for k, y in enumerate(range(int(far_y(80)) + 5, H, 4)):     # ripples
        xs = [x for x in range(W) if water[y, x]]
        if len(xs) > 6:
            x0 = xs[0] + 2 + (k % 2) * 2
            c.line(x0, y, x0 + 2, y)
            if len(xs) > 12:
                c.line(x0 + 6, y + 1, x0 + 8, y + 1)
    # footbridge: an arched deck, posts and a rail
    xs = list(range(BRIDGE_X0, BRIDGE_X1 + 1))
    top = [(x, deck_y(x)) for x in xs]
    G.white(c, [(BRIDGE_X0, BRIDGE_Y + 3), (BRIDGE_X1, BRIDGE_Y + 3)] + top[::-1])
    c.fill(top + [(x, y + 2) for x, y in top[::-1]])
    for x in range(BRIDGE_X0 + 1, BRIDGE_X1, 4):
        c.line(x, deck_y(x) - 1, x, deck_y(x) - 5)
    c.poly([(x, deck_y(x) - 6) for x in xs])


def well(c, x=108):
    y = int(near_y(x)) + 1
    G.white(c, [(x - 7, y), (x + 7, y), (x + 7, y - 8), (x - 7, y - 8)])
    for yy, off in ((y - 3, 0), (y - 6, 2)):
        for xx in range(x - 6 + off, x + 7, 4):
            c.line(xx, yy, xx, yy + 2)
    c.line(x - 7, y - 3, x + 7, y - 3)
    c.line(x - 7, y - 6, x + 7, y - 6)
    c.line(x - 6, y - 8, x - 6, y - 18)
    c.line(x + 6, y - 8, x + 6, y - 18)
    c.fill([(x - 10, y - 18), (x + 10, y - 18), (x, y - 25)])
    c.line(x - 6, y - 14, x + 6, y - 14)
    c.line(x + 1, y - 14, x + 1, y - 11)
    G.frame(c, x - 1, y - 11, x + 3, y - 9)


def cottage(c, x=108):
    """A friend's round-roofed cottage on the far hill, with smoke and a path."""
    y = int(far_y(x)) + 1
    G.white(c, [(x - 9, y), (x + 9, y), (x + 9, y - 8), (x - 9, y - 8)])
    c.fill([(x - 12, y - 7), (x + 12, y - 7), (x + 8, y - 13), (x + 2, y - 16), (x - 4, y - 16), (x - 9, y - 12)])
    for xx in range(x - 9, x + 10, 3):
        c.px(xx, y - 9, 0)
    G.white(c, [(x - 3, y), (x - 3, y - 5), (x - 1, y - 6), (x + 1, y - 5), (x + 1, y)])
    G.frame(c, x + 3, y - 6, x + 6, y - 3)
    c.fill([(x + 5, y - 13), (x + 7, y - 13), (x + 7, y - 18), (x + 5, y - 18)])
    for dx, dy in ((7, -20), (8, -22), (7, -24), (9, -26)):
        c.px(x + dx, y + dy)


def wide_scene(year, sky, people):
    """people: list of (sprite, x, on) where on is 'near', 'far' or (x, y)."""
    level = max(1, min(6, year - 4))
    back = T.Canvas()
    # far hill (solid) and the near ground line with a few ruled lines below
    for x in range(W):
        back.px(x, int(far_y(x)))
        back.px(x, int(near_y(x)))
    rng = random.Random(4)
    for dy in (4, 7, 11):
        x = rng.randint(-3, 3)
        while x < W:
            ln = rng.randint(3, 9)
            if rng.random() < 0.5:
                for xx in range(max(0, x), min(W, x + ln)):
                    yy = int(near_y(xx)) + dy
                    if yy < H:
                        back.px(xx, yy)
            x += ln + rng.randint(2, 6)
    for x in (6, 20, 58, 116):  # grass tufts
        gy = int(near_y(x))
        back.line(x, gy - 1, x - 1, gy - 3)
        back.line(x, gy - 1, x + 1, gy - 4)
    for x in (66, 112, 30):  # tufts on the far hill
        gy = int(far_y(x))
        back.line(x, gy - 1, x + 1, gy - 3)
    fg = T.Canvas()
    if year >= BRIDGE:
        bridge(fg)
    if year >= COTTAGE:
        cottage(fg)
    if year >= WELL:
        well(fg)
    wide_tree(fg, 34, level, random.Random(77))
    for sprite, x, on in people:
        y = near_y(x) if on == "near" else far_y(x) if on == "far" else deck_y(x) - 1 if on == "bridge" else on
        G.stamp(fg, sprite, x, int(y) - 0)
    region = T.grown(T.grown((fg.a | fg.keepout).astype(np.uint8))).astype(bool)
    back.a[region] = 0
    back.a |= fg.a
    mask = T.Canvas()
    mask.a[region] = 1
    for x in range(W):
        mask.a[int(far_y(x)):, x] = 1
    return T.compose(T.draw_sky(sky), back, mask)


# ---------------------------------------------------------------- the other two views (from make_gnome)

def middle_scene(year, sky, draw_fn):
    level = max(1, min(6, year - 4))
    return G.scene(20, sky, lambda c: (G.big_house(c, 62, 68, level), draw_fn(c)))


def close_scene(sky, draw_fn):
    c = T.Canvas()
    r = G.zoom_house(c)
    draw_fn(c, r)
    return c.image()


# ---------------------------------------------------------------- sample weeks

def week_sheet(days, name, out, title):
    k = 3
    pad, label = 8, 34
    cols = len(days)
    sheet = Image.new("L", ((W * k + pad) * cols - pad, H * k + label + 22), 255)
    d = ImageDraw.Draw(sheet)
    try:
        f = ImageFont.truetype("DejaVuSans.ttf", 13)
        fb = ImageFont.truetype("DejaVuSans-Bold.ttf", 15)
    except OSError:
        f = fb = ImageFont.load_default()
    d.text((2, 2), title, fill=0, font=fb)
    for i, (day, view, why, im) in enumerate(days):
        x = i * (W * k + pad)
        sheet.paste(im.convert("L").resize((W * k, H * k), Image.NEAREST), (x, 22))
        d.text((x + 2, 22 + H * k + 3), f"{day} · {view}", fill=0, font=fb)
        d.text((x + 2, 22 + H * k + 20), why, fill=0, font=f)
    sheet.save(os.path.join(out, name))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    gy = T.ground_y
    M, MP, MC, MD = G.MINI, G.MINI_PARTNER, G.MINI_CAT, G.MINI_DOG

    y8 = [
        ("Mon", "wide", "clear day, out together", wide_scene(8, 0, [(M, 70, "near"), (MP, 80, "near"), (MD, 92, "near"), (MC, 62, "near")])),
        ("Tue", "close", "rain: all in by the fire", close_scene(8, lambda c, r: (
            G.gnome(c, r["living"][0] - 1, r["living"][1], "sit", 1),
            G.gnome(c, r["kitchen"][0], r["kitchen"][1], "stand", -1, partner=True),
            G.stamp(c, G.CAT, r["hearth"][0] + 6, r["hearth"][1]), G.stamp(c, G.DOG, r["bed"][0] + 14, r["bed"][1] + 11)))),
        ("Wed", "middle", "watering the garden", middle_scene(8, 1, lambda c: (G.gnome(c, 14, gy(14), "water", 1), G.stamp(c, G.CAT, 104, gy(104))))),
        ("Thu", "close", "fog: reading upstairs", close_scene(11, lambda c, r: (
            G.gnome(c, r["study"][0], r["study"][1], "read", -1),
            G.gnome(c, r["living"][0] - 1, r["living"][1], "nap", 1, partner=True),
            G.stamp(c, G.CAT, r["bed"][0], r["bed"][1])))),
        ("Fri", "middle", "windy: raking leaves", middle_scene(8, 6, lambda c: (G.gnome(c, 14, gy(14), "rake", 1), G.stamp(c, G.DOG, 112, gy(112) + 1)))),
        ("Sat", "wide", "rainbow walk", wide_scene(8, 12, [(M, 84, "near"), (MP, 94, "near"), (MD, 106, "near")])),
        ("Sun", "close", "quiet tea", close_scene(5, lambda c, r: (
            G.gnome(c, r["kitchen"][0], r["kitchen"][1], "sit", 1),
            G.stamp(c, G.DOG, r["living"][0], r["living"][1]), G.stamp(c, G.CAT, r["bed"][0], r["bed"][1])))),
    ]
    week_sheet(y8, "world-week-y8@3x.png", out, "A week at 8 years: the camera moves with the day")

    y15 = [
        ("Mon", "wide", "new: a friend's cottage!", wide_scene(15, 4, [(M, 94, "far")])),
        ("Tue", "middle", "fair: waving hello", middle_scene(10, 1, lambda c: (G.gnome(c, 14, gy(14), "wave", 1), G.gnome(c, 33, gy(33), "stand", -1, partner=True)))),
        ("Wed", "close", "storm: by the fire", close_scene(9, lambda c, r: (
            G.gnome(c, r["living"][0] - 1, r["living"][1], "sit", 1),
            G.gnome(c, r["study"][0], r["study"][1], "read", -1, partner=True),
            G.stamp(c, G.CAT, r["hearth"][0] + 6, r["hearth"][1]), G.stamp(c, G.DOG, r["bed"][0] + 14, r["bed"][1] + 11)))),
        ("Thu", "wide", "drawing water", wide_scene(15, 1, [(M, 96, "near"), (MD, 84, "near")])),
        ("Fri", "middle", "windy: raking", middle_scene(10, 6, lambda c: (G.gnome(c, 14, gy(14), "rake", 1), G.stamp(c, G.CAT, 104, gy(104))))),
        ("Sat", "wide", "crossing the bridge", wide_scene(15, 2, [(M, 67, "bridge"), (MP, 77, "bridge")])),
        ("Sun", "close", "a cozy evening", close_scene(0, lambda c, r: (
            G.gnome(c, r["study"][0], r["study"][1], "read", -1),
            G.gnome(c, r["kitchen"][0], r["kitchen"][1], "stand", -1, partner=True),
            G.stamp(c, G.CAT, r["bed"][0], r["bed"][1]), G.stamp(c, G.DOG, r["living"][0], r["living"][1])))),
    ]
    week_sheet(y15, "world-week-y15@3x.png", out, "A week at 15 years: bridge (11), well (13) and a friend's cottage (15)")


if __name__ == "__main__":
    main()
