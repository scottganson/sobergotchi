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


HEAD = [  # '#' ink, '.' paper, ' ' see-through. Cap, eyes, round nose, beard.
    "      ####   ",
    "     ######  ",
    "     ###  ## ",
    "    ####  ## ",
    "    ####  ###",
    "    #####  ##",
    "   ######    ",
    "   #######   ",
    "  ########   ",
    "  #########  ",
    " ##########  ",
    " #.#.#.#.#.# ",
    "#############",
    " #.........# ",
    " #...###...# ",
    " #..#...#..# ",
    " #.#.###.#.# ",
    " #..#...#..# ",
    " #.........# ",
    "  #..#.#..#  ",
    "  #.......#  ",
    "   #.....#   ",
    "    #...#    ",
    "     ###     ",
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


def gnome(c, x, y, pose="stand", face=1):
    """Gnome with his feet at (x, y). face = 1 looks right, -1 looks left."""
    f = face
    x, y = int(round(x)), int(round(y))
    sit = pose == "sit"
    if pose == "cheer":
        y -= 2
    by = y - (0 if sit else 2)          # bottom of the tunic
    sh = by - 9                         # shoulders
    if sit:                             # legs out in front, toes up
        c.taper([(x + f * 3, by - 1), (x + f * 11, by - 1)], 3, 3)
        c.taper([(x + f * 11, by - 1), (x + f * 12, by - 4)], 2.6, 2.6)
    else:
        for s_ in (-1, 1):
            c.line(x + s_ * 3, by, x + s_ * 3, y - 1)
            toe = s_ if pose == "cheer" else f
            for dx in (0, 1, 2):
                c.px(x + s_ * 3 + toe * dx - (1 if toe > 0 else -1), y)
                c.px(x + s_ * 3 + toe * dx - (1 if toe > 0 else -1), y - 1) if dx < 2 else None
    c.fill([(x - 5, sh), (x + 5, sh), (x + 6, by), (x - 6, by)])   # tunic
    if pose == "stand":
        c.taper([(x - 5, sh + 1), (x - 8, sh + 5), (x - 8, sh + 8)], 2.4, 2.2)
        c.taper([(x + 5, sh + 1), (x + 8, sh + 5), (x + 8, sh + 8)], 2.4, 2.2)
    elif pose == "water":
        c.taper([(x + f * 5, sh + 2), (x + f * 9, sh + 4), (x + f * 11, sh + 3)], 2.4, 2.2)
        c.taper([(x - f * 5, sh + 1), (x - f * 8, sh + 5), (x - f * 8, sh + 8)], 2.4, 2.2)
        cx, cy = x + f * 15, sh + 4      # watering can
        white(c, [(cx - 3, cy - 3), (cx + 3, cy - 3), (cx + 3, cy + 3), (cx - 3, cy + 3)])
        c.line(cx + f * 3, cy + 2, cx + f * 8, cy - 2)             # spout
        c.line(cx + f * 8, cy - 4, cx + f * 8, cy - 1)
        c.poly(T.arc_curve((cx - 2, cy - 3), (cx + 2, cy - 3), 3 if f < 0 else -3, 6))  # handle
        for dx, dy in ((10, 0), (12, 2), (11, 5), (13, 7), (10, 8), (13, 11), (11, 13)):
            c.px(cx + f * dx, cy + dy)
    elif pose == "cheer":
        c.taper([(x - 5, sh + 1), (x - 9, sh - 2), (x - 10, sh - 6)], 2.4, 2.2)
        c.taper([(x + 5, sh + 1), (x + 9, sh - 2), (x + 10, sh - 6)], 2.4, 2.2)
        px = x + f * 10                   # pennant on a stick
        c.line(px, sh - 6, px, sh - 24)
        white(c, [(px, sh - 24), (px + f * 11, sh - 20), (px, sh - 16)])
        c.px(px + f * 3, sh - 20)
        c.px(px + f * 5, sh - 20)
    elif sit:
        c.taper([(x + f * 5, sh + 2), (x + f * 8, sh + 5), (x + f * 11, sh + 4)], 2.4, 2.2)
        cx, cy = x + f * 14, sh + 3      # a warm cup
        white(c, [(cx - 2, cy - 2), (cx + 2, cy - 2), (cx + 2, cy + 2), (cx - 2, cy + 2)])
        c.line(cx + f * 3, cy - 1, cx + f * 4, cy)
        c.px(cx + f * 3, cy + 1)
        for k in range(3):                # steam
            c.px(cx + (k % 2), cy - 4 - k * 2)
    stamp(c, HEAD, x, sh + 8, flip=f < 0)


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
        scene(17, 1, lambda c: gnome(c, 96, gy(96), "stand", -1)),
        scene(8, 0, lambda c: gnome(c, 30, gy(30), "water", 1)),
        scene(16, 4, lambda c: (gnome(c, 93, gy(93), "cheer", 1),
                                [c.px(x, y) for x, y in ((78, 64), (81, 54), (116, 60), (112, 70), (87, 46), (118, 48), (76, 74))])),
        scene(21, 7, lambda c: gnome(c, 72, gy(72), "sit", 1)),
    ]
    pad = 6
    sheet = Image.new("1", (len(panels) * (W + pad) - pad, H), 1)
    for k, im in enumerate(panels):
        sheet.paste(im, (k * (W + pad), 0))
    sheet.save(os.path.join(out, "gnome-poses.png"))
    sheet.convert("L").resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(os.path.join(out, "gnome-poses@3x.png"))
    # close-up of the standing gnome
    c = T.Canvas()
    gnome(c, 20, 40, "stand", 1)
    gnome(c, 52, 40, "cheer", -1)
    c.image().crop((4, 0, 76, 44)).convert("L").resize((72 * 8, 44 * 8), Image.NEAREST).save(os.path.join(out, "gnome-closeup.png"))


if __name__ == "__main__":
    main()
