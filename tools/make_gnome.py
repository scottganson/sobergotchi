#!/usr/bin/env python3
"""Concept sketches of the garden gnome, in the same 1-bit pen-and-ink style as the tree.

    python3 tools/make_gnome.py OUT_DIR     # writes gnome-poses.png (preview only, not firmware art)
"""
import math
import os
import random
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_tree_ink as T  # noqa: E402

W, H = T.W, T.H


def white(c, pts):
    """Paper-coloured shape with a fine ink outline."""
    tmp = T.Canvas()
    tmp.fill(pts)
    m = tmp.a.astype(bool)
    c.a[m] = 0
    c.keepout |= m
    c.poly(list(pts) + [pts[0]])


FACE = [  # '#' ink, '.' paper. Eyes, round nose, cheeks and moustache; sits between brim and beard.
    "#...........#",
    "#...........#",
    "#..#.....#..#",
    "#..#.###.#..#",
    "#...#...#...#",
    "#....###....#",
    "#..###.###..#",
]


def stamp(c, rows, cx, bottom, flip=False):
    """Place a hand-drawn sprite with its centre column at cx and last row at `bottom`."""
    h, w = len(rows), len(rows[0])
    for r, row in enumerate(rows):
        for k, ch in enumerate(row[::-1] if flip else row):
            px, py = int(round(cx)) - w // 2 + k, int(round(bottom)) - (h - 1) + r
            if not (0 <= px < W and 0 <= py < H) or ch == " ":
                continue
            c.a[py, px] = 1 if ch == "#" else 0
            if ch == ".":
                c.keepout[py, px] = True


def tube(c, pts, w=3.6):
    """Outlined sleeve or leg: an ink stroke with a paper core."""
    c.taper(pts, w, w)
    core = T.Canvas()
    core.taper(pts[:-1] + [((pts[-1][0] * 3 + pts[-2][0]) / 4, (pts[-1][1] * 3 + pts[-2][1]) / 4)], w - 2, w - 2)
    m = core.a.astype(bool)
    c.a[m] = 0
    c.keepout |= m


def boot(c, x, y, d):
    """Boot with its heel at x and toe pointing d (+1 right, -1 left)."""
    c.fill([(x - d * 1.5, y - 3), (x + d * 1.5, y - 3), (x + d * 4.5, y - 1), (x + d * 4.5, y), (x - d * 1.5, y)])


def gnome(c, x, y, pose="stand", face=1):
    """Gnome with his feet at (x, y). face = 1 looks right, -1 looks left."""
    f = face
    x, y = int(round(x)), int(round(y))
    sit = pose == "sit"
    if pose == "cheer":
        y -= 2
        for dx in (-7, -4, 4, 7):          # little hop marks
            c.px(x + dx, y + 2)
    by = y - (1 if sit else 3)           # hem of the tunic
    sh = by - 9                          # shoulders
    # legs and boots
    if sit:
        tube(c, [(x + f * 4, by - 2), (x + f * 12, by - 2)], 3.6)
        c.fill([(x + f * 12, by), (x + f * 12, by - 6), (x + f * 14.5, by - 6), (x + f * 14.5, by)])
    elif pose == "cheer":
        boot(c, x - 3, y, -1)
        boot(c, x + 3, y, 1)
    else:
        boot(c, x - 3, y, -1)
        boot(c, x + 3, y, 1)
    # far arm first, so the body overlaps it
    arms = {
        "stand": ([(-6, 1), (-9, 4), (-9, 7)], [(6, 1), (9, 4), (9, 7)]),
        "water": ([(-6, 1), (-9, 4), (-9, 7)], [(6, 2), (10, 5), (13, 4)]),
        "cheer": ([(-6, 1), (-11, -1), (-14, -6)], [(6, 1), (11, -1), (14, -6)]),
        "sit": ([(-6, 1), (-8, 5), (-7, 8)], [(6, 2), (9, 6), (13, 5)]),
    }[pose]
    hands = []
    for arm in arms:
        pts = [(x + f * dx, sh + dy) for dx, dy in arm]
        tube(c, pts, 3.6)
        hands.append(pts[-1])
    # tunic: paper with a fine outline, dark belt and buckle
    white(c, [(x - 6, sh), (x + 6, sh), (x + 8, by), (x - 8, by)])
    for xx in range(x - 7, x + 8):
        c.px(xx, by - 3)
        c.px(xx, by - 4)
    c.px(x, by - 3, 0)
    c.px(x, by - 4, 0)
    
    for hx, hy in hands:                   # mittens
        c.disc(hx, hy, 1.9)
    # props
    if pose == "water":
        hx, hy = hands[1]
        cx, cy = hx + f * 4, hy + 1
        white(c, [(cx - 3, cy - 3), (cx + 3, cy - 3), (cx + 3, cy + 3), (cx - 3, cy + 3)])
        c.line(cx - 3, cy, cx + 3, cy)
        c.line(cx + f * 3, cy + 2, cx + f * 8, cy - 2)
        c.line(cx + f * 8, cy - 4, cx + f * 8, cy - 1)
        c.poly(T.arc_curve((cx - 2, cy - 3), (cx + 2, cy - 3), 3 if f < 0 else -3, 6))
        for dx, dy in ((10, 0), (12, 2), (11, 5), (13, 7), (10, 8), (13, 11), (11, 13)):
            c.px(cx + f * dx, cy + dy)
    elif pose == "cheer":
        hx, hy = hands[1]
        c.line(hx, hy, hx, hy - 20)
        white(c, [(hx, hy - 20), (hx + f * 12, hy - 16), (hx, hy - 12)])
        c.disc(hx + f * 4, hy - 16, 1.2)
    elif sit:
        hx, hy = hands[1]
        cx, cy = hx + f * 4, hy - 1
        white(c, [(cx - 2, cy - 2), (cx + 2, cy - 2), (cx + 2, cy + 2), (cx - 2, cy + 2)])
        c.line(cx + f * 3, cy - 1, cx + f * 4, cy)
        c.px(cx + f * 3, cy + 1)
        for k in range(4):                 # steam
            c.px(cx + (k % 2), cy - 4 - k * 2)
    # beard: wide and soft, flowing over the tunic to a point
    top = sh - 2
    white(c, [(x - 6, top), (x + 6, top), (x + 6, top + 3), (x + 4.5, top + 7), (x + 1.5, top + 9.5), (x, top + 11),
              (x - 1.5, top + 9.5), (x - 4.5, top + 7), (x - 6, top + 3)])
    for dx, y0, y1 in ((-3, 3, 6), (0, 4, 8), (3, 3, 6)):
        c.line(x + dx, top + y0, x + dx * 0.6, top + y1)
    # face
    stamp(c, FACE, x, top, flip=f < 0)
    # cap: wide soft brim line, tall cone, tip flopping over with a little bobble
    brim = top - 7
    c.fill([(x - 8, brim + 1), (x + 8, brim + 1), (x + 7, brim - 1), (x + 3.5, brim - 9), (x + 1.5, brim - 14),
            (x - 1.5, brim - 14), (x - 3.5, brim - 9), (x - 7, brim - 1)])
    c.px(x - 9, brim)
    c.px(x + 9, brim)
    tip = (x, brim - 13)
    c.taper(T.bez(tip, (x - f * 1, brim - 19), (x - f * 8, brim - 19), (x - f * 9, brim - 12), 18), 3.4, 1.6)
    bx, by2 = x - f * 9, brim - 10
    c.disc(bx, by2, 2.3)
    c.disc(bx, by2, 1.1, 0)
    for xx in range(x - 6, x + 7):         # hat band
        if (xx - x) % 2 == 0:
            c.px(xx, brim - 2, 0)
    c.poly([(x + f * 2, brim - 5), (x + f * 1, brim - 9), (x + f * 0.5, brim - 12)], 0)  # light on the cap


def scene(stage, sky, draw_fn):
    plant, mask = T.draw(stage)
    g = T.Canvas()
    draw_fn(g)
    region = T.grown(T.grown((g.a | g.keepout).astype(np.uint8))).astype(bool)
    plant.a[region] = 0
    plant.a[g.a.astype(bool)] = 1
    mask.a[region] = 1
    return T.compose(T.draw_sky(sky), plant, mask)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    gy = lambda x: T.ground_y(x)
    panels = [
        scene(17, 1, lambda c: gnome(c, 98, gy(98), "stand", -1)),
        scene(8, 0, lambda c: gnome(c, 26, gy(26), "water", 1)),
        scene(16, 4, lambda c: (gnome(c, 93, gy(93), "cheer", 1),
                                [c.px(x, y) for x, y in ((78, 64), (81, 54), (116, 60), (112, 70), (87, 46), (118, 48), (76, 74))])),
        scene(21, 7, lambda c: gnome(c, 74, gy(74), "sit", 1)),
    ]
    pad = 6
    sheet = Image.new("1", (len(panels) * (W + pad) - pad, H), 1)
    for k, im in enumerate(panels):
        sheet.paste(im, (k * (W + pad), 0))
    sheet.save(os.path.join(out, "gnome-poses.png"))
    sheet.convert("L").resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(os.path.join(out, "gnome-poses@3x.png"))
    # close-up of the standing gnome
    c = T.Canvas()
    gnome(c, 18, 46, "stand", 1)
    gnome(c, 50, 46, "water", 1)
    gnome(c, 96, 46, "cheer", -1)
    gnome(c, 20, 100, "sit", 1)
    c.image().crop((0, 0, 122, 104)).convert("L").resize((122 * 6, 104 * 6), Image.NEAREST).save(os.path.join(out, "gnome-closeup.png"))


if __name__ == "__main__":
    main()
