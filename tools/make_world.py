#!/usr/bin/env python3
"""Concept: the camera changes day by day. The years decide what exists (the house grows from five to ten years,
then a new building arrives every few years); each day picks a view: wide (the whole community), middle (the tree
and house) or close (inside a room).

    python3 tools/make_world.py            # writes art/world, art/home (+ masks) for the firmware
    python3 tools/make_world.py OUT_DIR    # also previews: world-device@2x.png and two sample weeks

Run after make_tree_ink.py and make_gnome.py.
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


def wide_house(h, bx, floor, level):
    """The crooked storybook house in miniature: leaning walls, sagging shingled roof, round window, arched door,
    and for the later years an upper floor that overhangs, an attic window and a little deck. Returns half width."""
    hw = 9 + level
    L, R = bx - hw, bx + hw
    top = floor - 10
    G.white(h, [(L - 1, floor), (R + 1, floor), (R, top), (L, top)])          # walls lean out at the foot
    for dx in (0,):
        h.line(L - 1, floor, L, top)
        h.line(R + 1, floor, R, top)
    # arched door on the right with a step, round window on the left, a square one between when there is room
    G.white(h, [(R - 6, floor), (R - 6, floor - 5), (R - 5, floor - 7), (R - 3, floor - 7), (R - 2, floor - 5), (R - 2, floor)])
    h.px(R - 3, floor - 3)
    h.disc(L + 5, floor - 5, 3.2)
    h.disc(L + 5, floor - 5, 2.2, 0)
    h.px(L + 5, floor - 5)
    if hw >= 12:
        G.frame(h, bx - 2, floor - 7, bx + 2, floor - 3)
        h.line(bx, floor - 7, bx, floor - 3)
    h.fill([(L - 3, floor), (R + 3, floor), (R + 3, floor + 1), (L - 3, floor + 1)])   # floor beam
    cx, span = bx, hw + 4
    if level >= 3:                                   # upper floor, set off to the right and overhanging
        ux0, ux1 = L + 4, R + 3
        G.white(h, [(ux0, top), (ux1, top), (ux1 - 1, top - 8), (ux0 + 1, top - 8)])
        h.fill([(L - 3, top + 1), (ux0, top + 1), (ux0, top - 3)])          # little skirt roof
        h.fill([(ux0, top), (ux1 + 1, top), (ux1 + 1, top + 1), (ux0, top + 1)])
        G.frame(h, ux0 + 3, top - 6, ux0 + 6, top - 3)
        if level >= 4:
            G.frame(h, ux1 - 7, top - 6, ux1 - 4, top - 3)
        top -= 8
        cx, span = (ux0 + ux1) // 2, (ux1 - ux0) // 2 + 4
    rise = 9 + level
    px_ = cx - 2                                     # peak sits a little off centre
    left, right, base = cx - span, cx + span, top + 1
    roof = [(left - 1, base), (right + 1, base), (right - 1, base - 2)]
    for t in (0.25, 0.5, 0.75):
        roof.append((right + (px_ + 1 - right) * t - 1.5 * math.sin(math.pi * t), base + (top - rise - base) * t + 1.5 * math.sin(math.pi * t)))
    roof += [(px_ + 1, top - rise), (px_ - 1, top - rise)]
    for t in (0.75, 0.5, 0.25):
        roof.append((left + (px_ - 1 - left) * t + 1.5 * math.sin(math.pi * t), base + (top - rise - base) * t + 1.5 * math.sin(math.pi * t)))
    roof.append((left + 1, base - 2))
    tmp = T.Canvas()
    tmp.fill(roof)
    rm = tmp.a.astype(bool)
    h.a[rm] = 1
    for row in range(1, rise // 3 + 1):              # shingles
        yy = base - row * 3
        for xx in range(left, right):
            if (xx + row * 2) % 4 == 0 and rm[yy, xx] and rm[yy, xx - 1] and rm[yy, xx + 1] and rm[yy - 1, xx]:
                h.px(xx, yy, 0)
    h.px(px_, top - rise - 1)
    h.px(px_, top - rise - 2)
    if level >= 5:                                   # attic window
        h.disc(px_, top - rise // 2 + 1, 2.6, 0)
        h.px(px_, top - rise // 2 + 1)
    k = cx + span // 2 + 1                           # leaning chimney and smoke
    h.fill([(k, top - 3), (k + 3, top - 3), (k + 5, top - rise + 1), (k + 2, top - rise + 1)])
    h.line(k + 1, top - rise, k + 6, top - rise)
    for dx, dy in ((5, -3), (6, -5), (8, -6), (8, -8)):
        h.px(k + dx, top - rise + dy)
    if level >= 6:                                   # little deck with a rail on the left
        d0, d1 = L - 9, L - 2
        h.fill([(d0, floor), (d1, floor), (d1, floor + 1), (d0, floor + 1)])
        h.line(d0, floor - 4, d1, floor - 4)
        for xx in range(d0, d1 + 1, 3):
            h.line(xx, floor - 4, xx, floor - 1)
    return hw


def wide_tree(c, bx, level, rng):
    """A small tree with the house in its crown, sized for the wide view. level 1-6 = years 5-10."""
    by = int(near_y(bx))
    # trunk with flared roots
    c.fill([(bx - 9, by), (bx + 9, by), (bx + 5, by - 4), (bx + 4, by - 24), (bx - 4, by - 24), (bx - 5, by - 4)])
    c.line(bx - 1, by - 3, bx - 2, by - 16, 0)
    c.px(bx + 2, by - 10, 0)
    # the house sits on the trunk
    floor = by - 26
    house = T.Canvas()
    hw = wide_house(house, bx, floor, level)
    lx = max(bx + hw - 4, bx + 7)                   # ladder down the right side
    house.line(lx, floor + 2, lx, by - 1)
    house.line(lx + 3, floor + 2, lx + 3, by - 1)
    for yy in range(floor + 4, by, 3):
        house.line(lx, yy, lx + 3, yy)
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
    # ripples run with the current: short strokes down the stream, staggered
    rng = random.Random(8)
    for y in range(int(far_y(80)) + 4, H - 2, 3):
        xs = [x for x in range(W) if water[y, x] and water[y, x - 1] and water[y, x + 1]]
        if len(xs) < 3:
            continue
        # follow the river: its slope here, from the centre of the water a few rows down
        below = [x for x in range(W) if water[min(H - 1, y + 4), x]]
        dx = ((below[0] + below[-1]) / 2 - (xs[0] + xs[-1]) / 2) / 4 if below else -0.8
        for x in range(xs[0] + 1 + (y % 2) * 2, xs[-1] - 1, 4):
            x1, y1 = x + dx * 3, y + 3
            if rng.random() < 0.6 and y1 < H and water[int(y1), int(round(x1))]:
                c.line(x, y, x1, y1)
    # footbridge: an arched deck with the water flowing underneath, posts and a rail
    xs = list(range(BRIDGE_X0, BRIDGE_X1 + 1))
    top = [(x, deck_y(x)) for x in xs]
    band = T.Canvas()
    band.fill(top + [(x, y + 2) for x, y in top[::-1]])
    deck = T.grown(band.a).astype(bool)
    c.a[deck] = 0
    c.a[band.a.astype(bool)] = 1
    c.keepout |= deck
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
    """people: list of (sprite, x, on) where on is 'near', 'far', 'bridge' or a y value."""
    back, fg = wide_parts(year)
    for sprite, x, on in people:
        G.stamp(fg, sprite, x, int(where(x, on)))
    back, mask = _flatten(back, fg)
    return T.compose(T.draw_sky(sky), back, mask)


def where(x, on):
    return near_y(x) if on == "near" else far_y(x) if on == "far" else deck_y(x) - 1 if on == "bridge" else on


def wide_parts(year):
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
    return back, fg


def _flatten(back, fg):
    region = T.grown(T.grown((fg.a | fg.keepout).astype(np.uint8))).astype(bool)
    back.a[region] = 0
    back.a |= fg.a
    mask = T.Canvas()
    mask.a[region] = 1
    for x in range(W):
        mask.a[int(far_y(x)):, x] = 1
    return back, mask


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


# ---------------------------------------------------------------- firmware art
# From five years the tree layer is blank and two new groups take over. Order must match worldToday() in app.cpp.
#   art/world (+worldmask): 0 blank | 1-6 wide, house years 5-10 | 7 wide + bridge (11) | 8 + well (13) | 9 + cottage (15)
#                           | 10-15 middle, house years 5-10 | 16-25 close: 16 + 2 x weather (fine, rain, storm, snow,
#                             grey) + 1 with the robin (10 years)
#   art/home (+homemask):   0 nobody | 1-9 close: (gnome alone, + cat, + partner and dog) x 3 scenes
#                           | 10-21 wide: 2 scenes each for (alone, + cat, everyone, bridge, well, cottage)
#                           | 22 middle: the cat | 23 middle: cat and dog
# In the middle view the gnome himself is the usual pose layer (art/gnome), standing left of the tree.

def middle_layers(level):
    plant, mask = T.draw(20)
    g = T.Canvas()
    G.big_house(g, 62, 68, level)
    region = T.grown(T.grown((g.a | g.keepout).astype(np.uint8))).astype(bool)
    plant.a[region] = 0
    plant.a[g.a.astype(bool)] = 1
    mask.a[region] = 1
    return plant, mask


WEATHER = ["fine", "rain", "storm", "snow", "grey"]   # what the close view shows through its windows


def close_layers(robin=False, weather="fine"):
    """The close-up fills the frame, so the day's weather is drawn into its windows: the attic and kitchen panes."""
    c = T.Canvas()
    rooms = G.zoom_house(c)
    if robin:
        c.disc(58, 15, 6.5, 0)
        G.stamp(c, G.BIRD, 58, 20)
    yy, xx = np.mgrid[0:H, 0:W]
    panes = np.zeros((H, W), bool)
    if not robin:
        panes |= (xx - 58) ** 2 + (yy - 15) ** 2 <= 6.0 ** 2
    panes[77:89, 29:40] = True
    panes &= c.a == 0
    rng = random.Random(WEATHER.index(weather))
    for y in range(H):
        for x in range(W):
            if not panes[y, x]:
                continue
            if weather in ("rain", "storm"):
                step = 3 if weather == "storm" else 5
                if (x + y) % step == 0 and y % 3 == 0 and panes[min(H - 1, y + 1), max(0, x - 1)]:
                    c.px(x, y)
                    c.px(x - 1, y + 1)
            elif weather == "snow":
                if rng.random() < 0.13:
                    c.px(x, y)
            elif weather == "grey":
                if y % 3 == 0 and (x // 3 + y) % 3:
                    c.px(x, y)
    if weather == "storm" and not robin:            # a flash of lightning in the attic window
        c.poly([(60, 10), (57, 14), (60, 14), (56, 20)], 1)
    if weather == "snow":                           # snow on the kitchen sill
        c.line(28, 89, 40, 89)
        c.line(29, 88, 39, 88)
    mask = T.Canvas()
    mask.a[:, :] = 1
    return c, mask, rooms


def close_residents():
    """Three scenes for each household: gnome alone (5-6 years), with the cat (7), everyone (8 on)."""
    G_ = G.gnome
    alone = [
        lambda c, r: G_(c, r["living"][0] - 1, r["living"][1], "sit", 1),
        lambda c, r: G_(c, r["study"][0], r["study"][1], "read", -1),
        lambda c, r: G_(c, r["kitchen"][0], r["kitchen"][1], "wave", 1),
    ]
    cat = [lambda c, r: G.stamp(c, G.CAT, r["bed"][0], r["bed"][1]),
           lambda c, r: G.stamp(c, G.CAT, r["hearth"][0] + 6, r["hearth"][1]),
           lambda c, r: G.stamp(c, G.CAT, r["hearth"][0] + 6, r["hearth"][1])]
    everyone = [
        [alone[0], lambda c, r: G_(c, r["kitchen"][0], r["kitchen"][1], "stand", -1, partner=True), cat[0],
         lambda c, r: G.stamp(c, G.DOG, r["study"][0], r["study"][1])],
        [alone[1], lambda c, r: G_(c, r["living"][0] - 1, r["living"][1], "nap", 1, partner=True),
         lambda c, r: G.stamp(c, G.CAT, r["kitchen"][0] - 2, r["kitchen"][1]),
         lambda c, r: G.stamp(c, G.DOG, r["bed"][0] + 14, r["bed"][1] + 11)],
        [alone[2], lambda c, r: G_(c, r["study"][0], r["study"][1], "read", -1, partner=True), cat[1],
         lambda c, r: G.stamp(c, G.DOG, r["bed"][0] + 14, r["bed"][1] + 11)],
    ]
    return [[a] for a in alone] + [[alone[k], cat[k]] for k in range(3)] + everyone


M, MP, MC, MD = G.MINI, G.MINI_PARTNER, G.MINI_CAT, G.MINI_DOG
WIDE_PEOPLE = [   # (year the scene is drawn for, [(sprite, x, on)])
    (6, [(M, 66, "near")]), (6, [(M, 92, "near")]),
    (7, [(M, 66, "near"), (MC, 78, "near")]), (7, [(MC, 84, "near"), (M, 96, "near")]),
    (8, [(MC, 62, "near"), (M, 70, "near"), (MP, 80, "near"), (MD, 92, "near")]),
    (8, [(M, 84, "near"), (MP, 94, "near"), (MD, 106, "near")]),
    (11, [(M, 67, "bridge"), (MP, 77, "bridge")]),
    (11, [(MC, 92, "near"), (M, 100, "near"), (MP, 110, "near")]),
    (13, [(MD, 84, "near"), (M, 96, "near")]),
    (13, [(M, 67, "bridge"), (MP, 77, "bridge"), (MD, 116, "near")]),
    (15, [(M, 94, "far"), (MD, 84, "near")]),
    (15, [(MP, 72, "bridge"), (M, 96, "near")]),
]


def write_world_art(root):
    worlds, homes = [], []
    blank = T.Canvas()
    worlds.append((blank, blank))
    for year in (5, 6, 7, 8, 9, 10, 11, 13, 15):
        back, fg = wide_parts(year)
        worlds.append(_flatten(back, fg))
    for level in range(1, 7):
        worlds.append(middle_layers(level))
    for weather in WEATHER:
        for robin in (False, True):
            close, cmask, rooms = close_layers(robin, weather)
            worlds.append((close, cmask))

    def layer(draw):
        g = T.Canvas()
        draw(g)
        m = T.Canvas()
        m.a[(g.a | g.keepout).astype(bool)] = 1
        return g, m

    homes.append((blank, blank))
    for fns in close_residents():
        homes.append(layer(lambda g: [fn(g, rooms) for fn in fns]))
    for _, people in WIDE_PEOPLE:
        homes.append(layer(lambda g: [G.stamp(g, sp, x, int(where(x, on))) for sp, x, on in people]))
    homes.append(layer(lambda g: G.stamp(g, G.CAT, 86, T.ground_y(86))))
    homes.append(layer(lambda g: (G.stamp(g, G.CAT, 86, T.ground_y(86)), G.stamp(g, G.DOG, 113, T.ground_y(113) + 1))))
    for name, frames in (("world", worlds), ("home", homes)):
        for d in (name, name + "mask"):
            os.makedirs(os.path.join(root, "art", d), exist_ok=True)
            for f in os.listdir(os.path.join(root, "art", d)):
                os.remove(os.path.join(root, "art", d, f))
        for i, (img, mask) in enumerate(frames):
            img.image().save(os.path.join(root, "art", name, f"{name}-{i:02d}.png"))
            mask.image().save(os.path.join(root, "art", name + "mask", f"{name}-{i:02d}.png"))
    return worlds, homes


def device_sheet(worlds, homes, out):
    """Every home frame composed on its view, as the device draws it (sky, world, gnome pose, home)."""
    def comp(w, h, sky, pose=None):
        img, mask = worlds[w]
        a = img.a.copy()
        m = mask.a.copy()
        if pose:
            g = T.Canvas()
            G.gnome(g, G.HOME_X, T.ground_y(G.HOME_X), pose, 1)
            reg = T.grown(T.grown((g.a | g.keepout).astype(np.uint8))).astype(bool)
            a[reg] = 0
            a |= g.a
            m[reg] = 1
        hi, hm = homes[h]
        a[hm.a.astype(bool)] = 0
        a |= hi.a
        m |= hm.a
        p = T.Canvas()
        p.a = a
        mm = T.Canvas()
        mm.a = m
        return T.compose(T.draw_sky(sky), p, mm)

    wide_world = [2, 2, 3, 3, 4, 4, 7, 7, 8, 8, 9, 9]
    ims = [comp(16 + 2 * (k % 5), 1 + k, 0) for k in range(9)]
    ims += [comp(wide_world[k], 10 + k, k % 13) for k in range(12)]
    ims += [comp(10 + lv, (0, 0, 22, 23, 23, 23)[lv], lv, ("water", "rake", "wave", "read", "sit", "stand")[lv]) for lv in range(6)]
    ims += [comp(17 + 2 * k, 7, 0) for k in range(5)]
    pad, cols = 6, 7
    rows = (len(ims) + cols - 1) // cols
    sheet = Image.new("1", (cols * (W + pad) - pad, rows * (H + pad) - pad), 1)
    for k, im in enumerate(ims):
        sheet.paste(im, ((k % cols) * (W + pad), (k // cols) * (H + pad)))
    sheet.convert("L").resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(os.path.join(out, "world-device@2x.png"))


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    worlds, homes = write_world_art(root)
    if len(sys.argv) < 2:
        return
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    device_sheet(worlds, homes, out)
    sample_weeks(out)


def sample_weeks(out):
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
