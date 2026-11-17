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


FACE_PARTNER = [
    "#...........#",
    "#...........#",
    "#..#.....#..#",
    "#..#.....#..#",
    "#.....#.....#",
    "#.#.......#.#",
    "#....###....#",
]
CAT = [
    " #   #     ",
    " ## ##     ",
    " #####     ",
    " #.#.#     ",
    " #####     ",
    "  ###      ",
    "  ####     ",
    " ######    ",
    " #######  #",
    " ####### # ",
    " ####### # ",
    " ########  ",
]
DOG = [
    "  ###       ",
    " ######     ",
    "##.#####    ",
    "####### #   ",
    " ######     ",
    "   #####    ",
    "   ######  #",
    "   ####### #",
    "   ######## ",
    "   ######## ",
    "   ## ####  ",
]
BIRD = [
    "  ##   ",
    " #.##  ",
    "######  "[:7],
    " ##### ",
    "  #### ",
    "  # ## ",
]


def tree_house(c, x, y):
    """Cabin in the crown with its floor at (x, y): deck with railings, shingled roof, chimney, ladder."""
    # braces down to the trunk, then the deck
    c.line(x - 15, y + 2, x - 2, y + 13)
    c.line(x + 15, y + 2, x + 6, y + 13)
    c.fill([(x - 23, y), (x + 23, y), (x + 23, y + 1), (x - 23, y + 1)])
    for k in range(-22, 23, 4):                 # plank ends under the deck
        c.px(x + k, y + 2)
    # walls with plank lines
    white(c, [(x - 13, y), (x + 12, y), (x + 12, y - 17), (x - 13, y - 17)])
    for yy in (y - 6, y - 12):
        for xx in range(x - 12, x + 12):
            if xx % 2 == 0:
                c.px(xx, yy)
    # arched door with a knob and a step
    door = [(x - 10, y - 1), (x - 10, y - 9), (x - 9, y - 11), (x - 7, y - 12), (x - 5, y - 11), (x - 4, y - 9), (x - 4, y - 1)]
    white(c, door)
    c.line(x - 7, y - 11, x - 7, y - 1)
    c.px(x - 5, y - 6)
    # round window with a cross and a flower box
    c.disc(x + 5, y - 10, 4.2)
    c.disc(x + 5, y - 10, 3.0, 0)
    c.line(x + 5, y - 13, x + 5, y - 7)
    c.line(x + 2, y - 10, x + 8, y - 10)
    c.line(x + 1, y - 4, x + 9, y - 4)
    for k in (2, 4, 6, 8):
        c.px(x + k, y - 5)
    # chimney behind the roof, with smoke
    c.fill([(x + 6, y - 22), (x + 9, y - 22), (x + 9, y - 30), (x + 6, y - 30)])
    c.line(x + 5, y - 31, x + 10, y - 31)
    for k, (dx, dy) in enumerate(((8, -34), (9, -36), (11, -38), (11, -41), (13, -43))):
        c.px(x + dx, y + dy)
    # shingled roof with wide eaves
    c.fill([(x - 18, y - 17), (x + 17, y - 17), (x + 15, y - 19), (x, y - 32), (x - 1, y - 32), (x - 16, y - 19)])
    for row, yy in enumerate((y - 20, y - 23, y - 26, y - 29)):
        half = (32 - (y - yy)) * 1.15
        for xx in range(int(x - half) + 1, int(x + half)):
            if (xx + row) % 3 == 0:
                c.px(xx, yy, 0)
    c.px(x, y - 33)
    c.px(x - 1, y - 33)
    # railings either side of the cabin
    for x0, x1 in ((x - 23, x - 14), (x + 13, x + 23)):
        c.line(x0, y - 6, x1, y - 6)
        for xx in range(x0, x1 + 1, 3):
            c.line(xx, y - 6, xx, y - 1)
    # lantern under the deck
    c.line(x - 19, y + 2, x - 19, y + 4)
    white(c, [(x - 21, y + 5), (x - 17, y + 5), (x - 17, y + 9), (x - 21, y + 9)])
    c.px(x - 19, y + 7)
    # ladder from the end of the deck to the grass
    lx = x + 18
    gy = int(T.ground_y(lx))
    c.line(lx, y + 2, lx, gy)
    c.line(lx + 4, y + 2, lx + 4, gy)
    for yy in range(int(y) + 5, gy, 4):
        c.line(lx, yy, lx + 4, yy)


FACE_ASLEEP = FACE[:2] + ["#...........#", "#.##.###.##.#"] + FACE[4:]


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


def gnome(c, x, y, pose="stand", face=1, partner=False):
    """Gnome with his feet at (x, y). face = 1 looks right, -1 looks left."""
    f = face
    x, y = int(round(x)), int(round(y))
    sit = pose in ("sit", "read", "nap")
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
        "read": ([(-6, 1), (-8, 5), (-7, 8)], [(6, 2), (9, 6), (12, 4)]),
        "nap": ([(-6, 1), (-8, 5), (-6, 8)], [(6, 1), (8, 5), (6, 8)]),
        "wave": ([(-6, 1), (-9, 4), (-9, 7)], [(6, 1), (11, -2), (13, -8)]),
        "rake": ([(-6, 1), (-9, 4), (-9, 7)], [(6, 2), (10, 4), (12, 2)]),
        "lantern": ([(-6, 1), (-9, 4), (-9, 7)], [(6, 2), (10, 2), (13, 0)]),
        "umbrella": ([(-6, 1), (-9, 4), (-9, 7)], [(6, 2), (9, 2), (10, -1)]),
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
    elif pose == "wave":
        hx, hy = hands[1]
        for dx, dy in ((3, -3), (4, -1), (-3, -4)):       # little motion ticks
            c.px(hx + f * dx, hy + dy)
            c.px(hx + f * (dx + (1 if dx > 0 else -1)), hy + dy - 1)
    elif pose == "rake":
        hx, hy = hands[1]
        tx, ty = hx + f * 9, y              # rake head on the ground
        c.line(hx - f * 4, hy - 12, tx, ty - 1)
        c.line(tx - 3, ty - 1, tx + 3, ty - 1)
        for dx in (-3, -1, 1, 3):
            c.px(tx + dx, ty)
        for dx, dy, a in ((5, -2, 20), (9, -1, -30), (11, -3, 160), (7, -5, 80)):   # raked leaves
            c.a[T.leaf_mask(tx + f * dx, ty + dy, a, 5, 0.5)] = 1
    elif pose == "lantern":
        hx, hy = hands[1]
        lx, ly = hx + f * 1, hy + 3
        c.line(lx, hy, lx, ly)
        white(c, [(lx - 2, ly), (lx + 2, ly), (lx + 2, ly + 6), (lx - 2, ly + 6)])
        c.line(lx - 3, ly, lx + 3, ly)
        c.line(lx - 3, ly + 6, lx + 3, ly + 6)
        c.px(lx, ly + 3)
        c.px(lx, ly + 4)
        for a in range(0, 360, 45):          # glow
            c.px(lx + 6.5 * math.cos(math.radians(a + 22)), ly + 3 + 6.5 * math.sin(math.radians(a + 22)))
    elif pose == "umbrella":
        hx, hy = hands[1]
        ux, top_y = hx, sh - 29
        c.line(ux, hy, ux, top_y + 7)
        dome = [(ux - f * 6 + 18 * math.cos(math.radians(a)), top_y + 7 + 8 * math.sin(math.radians(a))) for a in range(180, 361, 12)]
        white(c, dome)
        for k in (-9, 0, 9):                 # ribs
            c.line(ux - f * 6, top_y - 1, ux - f * 6 + k, top_y + 7)
        c.line(ux - f * 6, top_y - 1, ux - f * 6, top_y - 3)
    elif pose == "read":
        hx, hy = hands[1]
        bx_, by_ = hx + f * 1, hy - 3
        white(c, [(bx_ - 5, by_ - 3), (bx_, by_ - 1), (bx_ + 5, by_ - 3), (bx_ + 5, by_ + 2), (bx_, by_ + 4), (bx_ - 5, by_ + 2)])
        c.line(bx_, by_ - 1, bx_, by_ + 4)
        for dx in (-3, 3):
            c.px(bx_ + dx, by_)
            c.px(bx_ + dx, by_ + 2)
    elif pose == "nap":
        for k, (dx, dy) in enumerate(((10, -22), (14, -27), (19, -33))):   # z z Z
            n = 2 + (k > 1)
            zx, zy = x + f * dx, sh + dy + 12
            c.line(zx, zy, zx + n, zy)
            c.line(zx + n, zy, zx, zy + n)
            c.line(zx, zy + n, zx + n, zy + n)
    elif pose == "sit":
        hx, hy = hands[1]
        cx, cy = hx + f * 4, hy - 1
        white(c, [(cx - 2, cy - 2), (cx + 2, cy - 2), (cx + 2, cy + 2), (cx - 2, cy + 2)])
        c.line(cx + f * 3, cy - 1, cx + f * 4, cy)
        c.px(cx + f * 3, cy + 1)
        for k in range(4):                 # steam
            c.px(cx + (k % 2), cy - 4 - k * 2)
    # beard: wide and soft, flowing over the tunic to a point
    top = sh - 2
    if partner:   # no beard: a round chin and two long braids
        white(c, [(x - 6, top), (x + 6, top), (x + 5, top + 3), (x + 2, top + 5), (x - 2, top + 5), (x - 5, top + 3)])
        for s_ in (-1, 1):
            for k in range(5):
                c.disc(x + s_ * 7, top + 1 + k * 2.4, 1.5 if k % 2 == 0 else 1.1)
            c.disc(x + s_ * 7, top + 13, 0.9)
    else:
        white(c, [(x - 6, top), (x + 6, top), (x + 6, top + 3), (x + 4.5, top + 7), (x + 1.5, top + 9.5), (x, top + 11),
                  (x - 1.5, top + 9.5), (x - 4.5, top + 7), (x - 6, top + 3)])
        for dx, y0, y1 in ((-3, 3, 6), (0, 4, 8), (3, 3, 6)):
            c.line(x + dx, top + y0, x + dx * 0.6, top + y1)
    # face
    stamp(c, FACE_PARTNER if partner else FACE_ASLEEP if pose == "nap" else FACE, x, top, flip=f < 0)
    # cap: wide soft brim line, tall cone, tip flopping over with a little bobble
    brim = top - 7
    if partner:   # a pale cap with a dark band and a flower
        white(c, [(x - 8, brim + 1), (x + 8, brim + 1), (x + 7, brim - 1), (x + 3.5, brim - 8), (x + 1, brim - 12),
                  (x - f * 3, brim - 14), (x - f * 6, brim - 11), (x - 3.5, brim - 7), (x - 7, brim - 1)])
        for xx in range(x - 7, x + 8):
            c.px(xx, brim - 1)
            c.px(xx, brim)
        fx, fy = x + f * 5, brim - 4
        for dx, dy in ((0, -2), (2, 0), (0, 2), (-2, 0), (-1, -1), (1, -1), (1, 1), (-1, 1)):
            c.px(fx + dx, fy + dy)
        return
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


POSES = ["stand", "wave", "water", "rake", "read", "sit", "nap", "lantern", "umbrella", "cheer"]  # order = firmware Pose enum
HOME_X = 24   # where he stands on the home screen, left of the tree


def write_art(root):
    """Firmware art: full-frame gnome layers for the home screen, their knock-out masks, and 2x close-ups for his page."""
    for d in ("gnome", "gnomemask", "gnomebig"):
        os.makedirs(os.path.join(root, "art", d), exist_ok=True)
    for i, name in enumerate(POSES):
        c = T.Canvas()
        gnome(c, HOME_X, T.ground_y(HOME_X), name, 1)
        c.image().save(os.path.join(root, "art", "gnome", f"pose-{i:02d}-{name}.png"))
        m = T.Canvas()
        m.a[T.grown(T.grown((c.a | c.keepout).astype(np.uint8))).astype(bool)] = 1
        m.image().save(os.path.join(root, "art", "gnomemask", f"pose-{i:02d}-{name}.png"))
        b = T.Canvas()
        gnome(b, 22, 56, name, 1)
        big = b.image().crop((0, 0, 60, 60)).convert("L").resize((120, 120), Image.NEAREST).convert("1")
        big.save(os.path.join(root, "art", "gnomebig", f"pose-{i:02d}-{name}.png"))


MINI = [   # the gnome seen at a distance, inside the house
    "    #    ",
    "   ###   ",
    "   ###   ",
    "  #####  ",
    "  #####  ",
    " ####### ",
    "#########",
    " #.....# ",
    " #..#..# ",
    " #.....# ",
    "  #...#  ",
    "  ##.##  ",
    "  ## ##  ",
]
MINI_PARTNER = [
    "    #    ",
    "   #.#   ",
    "   #.#   ",
    "  #...#  ",
    "  #...#  ",
    " #.....# ",
    "#########",
    " #.....# ",
    "##.#.#.##",
    "# #...# #",
    "  #####  ",
    "  #####  ",
    "  ## ##  ",
]
MINI_CAT = [
    "# #    ",
    "###   #",
    "###  # ",
    "#####  ",
    "#####  ",
]
MINI_DOG = [
    " ##     ",
    "####   #",
    "#.##  # ",
    " ###### ",
    " ###### ",
    " #    # ",
]


def rect(c, x0, y0, x1, y1):
    c.line(x0, y0, x1, y0)
    c.line(x1, y0, x1, y1)
    c.line(x1, y1, x0, y1)
    c.line(x0, y1, x0, y0)


def big_house(c, cx, base, level=6):
    """A storybook cutaway house that grows with the years until it fills the crown. It sits on a thick trunk;
    the upper floor is set to one side and overhangs, the roof sags and the chimney leans.
    level 1: one room. 2: two rooms. 3: upper floor with a bedroom. 4: study. 5: attic. 6: deck.
    base = y of the bottom floor beam. Returns the floor positions of the rooms for placing figures."""
    half = 17 if level == 1 else 32
    L, R = cx - half, cx + half
    f2 = base - 19                          # floor line of the upper floor
    two = level >= 3
    UL, UR = (L + 9, R + 5) if two else (L, R)   # the upper floor sits off to the right and overhangs
    f1 = base - 38 if two else f2           # underside of the roof
    ucx = (UL + UR) // 2
    rise = 21 if half == 32 else 14
    px_ = ucx - (5 if half == 32 else 3)    # the peak sits off-centre
    peak = f1 - rise

    # thick trunk with flared roots, bark lines and a knot
    gy = int(T.ground_y(cx)) + 1
    w = 9 if level >= 2 else 7
    c.fill([(cx - w - 3, base + 2), (cx + w + 3, base + 2), (cx + w, base + 7), (cx + w - 1, gy - 9), (cx + w + 2, gy - 3),
            (cx + w + 7, gy), (cx - w - 7, gy), (cx - w - 2, gy - 3), (cx - w + 1, gy - 9), (cx - w, base + 7)])
    c.poly([(cx - 3, base + 6), (cx - 4, base + 13), (cx - 3, gy - 5)], 0)
    c.poly([(cx + 4, base + 9), (cx + 3, gy - 8), (cx + 5, gy - 3)], 0)
    c.disc(cx + 0.5, base + 12, 1.6, 0)
    c.px(cx, base + 12)

    # shell: walls lean out a little toward the bottom
    white(c, [(L - 2, base), (R + 2, base), (R, f2), (L, f2)])
    if two:
        white(c, [(UL, f2), (UR, f2), (UR - 1, f1), (UL + 1, f1)])
        # little skirt roof over the part of the lower floor that sticks out on the left
        c.fill([(L - 4, f2 + 1), (UL, f2 + 1), (UL, f2 - 6)])
    e = (UR - UL) // 2 + 7
    left, right = ucx - e, ucx + e
    roof = [(left - 2, f1 + 1), (right + 2, f1 + 1), (right - 1, f1 - 2)]
    for t in (0.25, 0.5, 0.75):             # sagging slopes
        roof.append((right + (px_ + 1 - right) * t - 3 * math.sin(math.pi * t), f1 + (peak - f1) * t + 2.5 * math.sin(math.pi * t)))
    roof += [(px_ + 1, peak), (px_ - 1, peak)]
    for t in (0.75, 0.5, 0.25):
        roof.append((left + (px_ - 1 - left) * t + 3 * math.sin(math.pi * t), f1 + (peak - f1) * t + 2.5 * math.sin(math.pi * t)))
    roof.append((left + 1, f1 - 2))
    tmp = T.Canvas()
    tmp.fill(roof)
    roof_mask = tmp.a.astype(bool)
    c.a[roof_mask] = 1
    for row in range(1, rise // 3 + 1):     # shingles
        yy = f1 - row * 3 + 1
        for xx in range(left, right):
            if (xx + row * 2) % 4 == 0 and roof_mask[yy, xx] and roof_mask[yy, xx - 1] and roof_mask[yy, xx + 1] and roof_mask[yy - 1, xx]:
                c.px(xx, yy, 0)
    c.px(px_, peak - 1)                     # finial
    c.px(px_, peak - 2)
    if level >= 5:
        white(c, [(px_ - 17, f1 - 1), (px_ + 19, f1 - 1), (px_ + 1, peak + 10)])
    # floors
    c.fill([(L - 3, base), (R + 3, base), (R + 3, base + 1), (L - 3, base + 1)])
    c.fill([(L - 1, f2), (max(R, UR) + 1, f2), (max(R, UR) + 1, f2 + 1), (L - 1, f2 + 1)])
    if two:
        c.fill([(UL, f1), (UR, f1), (UR, f1 + 1), (UL, f1 + 1)])
    # walls, two pixels thick
    for dx in (0, 1):
        c.line(L - 2 + dx, base, L + dx, f2)
        c.line(R + 2 - dx, base, R - dx, f2)
        if two:
            c.line(UL + dx, f2, UL + 1 + dx, f1)
            c.line(UR - dx, f2, UR - 1 - dx, f1)
    if half == 32:                          # room dividers with doorways
        c.line(cx - 1, f2 + 2, cx - 1, base - 9)
        if two:
            c.line(ucx - 1, f1 + 2, ucx - 1, f2 - 9)
    # leaning chimney and smoke
    k = ucx + (21 if half == 32 else 9)
    top = f1 - (16 if half == 32 else 11)
    c.fill([(k, f1 - 4), (k + 4, f1 - 4), (k + 6, top), (k + 2, top)])
    c.line(k + 1, top - 1, k + 7, top - 1)
    for dx, dy in ((5, -4), (6, -6), (8, -8), (8, -11), (10, -13)):
        c.px(k + dx, top + dy)
    rooms = {}
    if level >= 5:
        attic_room(c, px_ + 1, f1)
        rooms["attic"] = (px_, f1 - 1)
    if two:
        bedroom(c, UL, f2)
        rooms["bed"] = (UL + 12, f2 - 7)
    if level >= 4:
        study(c, ucx, UR, f2)
        rooms["study"] = (ucx + 9, f2 - 1)
    kitchen(c, L, base)
    rooms["kitchen"] = (L + 30, base - 1)
    if half == 32:
        living(c, cx, R, f2, base)
        rooms["living"] = (cx + 10, base - 1)
    ladder_x = R + 4
    if level >= 6:                          # deck and railing on the right
        dx0, dx1 = R + 3, R + 14
        c.fill([(dx0, base), (dx1, base), (dx1, base + 1), (dx0, base + 1)])
        c.line(dx0, base - 6, dx1, base - 6)
        for xx in range(dx0 + 2, dx1 + 1, 3):
            c.line(xx, base - 6, xx, base - 1)
        ladder_x = dx1 - 4
        rooms["deck"] = (R + 9, base - 1)
    c.fill([(R, base - 9), (R + 1, base - 9), (R + 2, base - 1), (R, base - 1)], 0)        # doorway
    lg = int(T.ground_y(ladder_x))
    c.fill([(R, base), (ladder_x + 5, base), (ladder_x + 5, base + 1), (R, base + 1)])
    c.line(ladder_x, base + 2, ladder_x, lg)
    c.line(ladder_x + 4, base + 2, ladder_x + 4, lg)
    for yy in range(int(base) + 5, lg, 4):
        c.line(ladder_x, yy, ladder_x + 4, yy)
    return rooms


def attic_room(c, cx, f1):
    """A chest, a telescope and a star."""
    rect(c, cx - 12, f1 - 6, cx - 5, f1 - 2)
    c.line(cx - 12, f1 - 4, cx - 5, f1 - 4)
    c.px(cx - 8, f1 - 3)
    c.line(cx + 3, f1 - 3, cx + 10, f1 - 9)
    c.line(cx + 4, f1 - 3, cx + 11, f1 - 9)
    c.line(cx + 6, f1 - 5, cx + 4, f1 - 1)
    c.line(cx + 6, f1 - 5, cx + 9, f1 - 1)
    for dx, dy in ((0, -11), (-1, -10), (1, -10), (0, -9), (0, -10)):
        c.px(cx + dx, f1 + dy, 1 if (dx, dy) != (0, -10) else 0)



def bedroom(c, L, f2):
    x0 = L + 3
    c.fill([(x0, f2 - 10), (x0 + 1, f2 - 10), (x0 + 1, f2 - 1), (x0, f2 - 1)])           # headboard
    rect(c, x0 + 1, f2 - 6, x0 + 18, f2 - 3)                                                # mattress
    c.line(x0 + 18, f2 - 3, x0 + 18, f2 - 1)
    for xx in range(x0 + 8, x0 + 18):                                                      # quilt
        if xx % 2 == 0:
            c.px(xx, f2 - 5)
        else:
            c.px(xx, f2 - 4)
    c.line(x0 + 3, f2 - 7, x0 + 6, f2 - 7)                                                 # pillow
    rect(c, x0 + 21, f2 - 5, x0 + 25, f2 - 1)                                              # night stand
    c.line(x0 + 23, f2 - 8, x0 + 23, f2 - 6)                                               # candle
    c.px(x0 + 23, f2 - 10)
    rect(c, x0 + 9, f2 - 16, x0 + 15, f2 - 11)                                             # picture: a little tree
    c.px(x0 + 12, f2 - 13)
    c.px(x0 + 12, f2 - 14)
    c.px(x0 + 11, f2 - 14)
    c.px(x0 + 13, f2 - 14)



def study(c, cx, R, f2):
    """Bookshelves, a lamp and an armchair."""
    bx = R - 15
    rect(c, bx, f2 - 16, R - 3, f2 - 1)
    for yy in (f2 - 11, f2 - 6):
        c.line(bx, yy, R - 3, yy)
    rng = random.Random(4)
    for shelf in (f2 - 12, f2 - 7, f2 - 2):
        for xx in range(bx + 2, R - 4):
            if rng.random() < 0.7:
                c.line(xx, shelf, xx, shelf - rng.choice((2, 3, 3)))
    ax = cx + 4
    c.fill([(ax, f2 - 9), (ax + 1, f2 - 9), (ax + 1, f2 - 1), (ax, f2 - 1)])               # armchair back
    rect(c, ax + 1, f2 - 5, ax + 7, f2 - 2)
    c.line(ax + 7, f2 - 2, ax + 7, f2 - 1)
    lx = cx + 14
    c.line(lx, f2 - 1, lx, f2 - 11)                                                         # floor lamp
    c.fill([(lx - 3, f2 - 11), (lx + 3, f2 - 11), (lx + 1, f2 - 14), (lx - 1, f2 - 14)])



def kitchen(c, L, base):
    x0 = L + 3
    c.fill([(x0, base - 7), (x0 + 7, base - 7), (x0 + 7, base - 1), (x0, base - 1)])       # stove
    c.px(x0 + 2, base - 4, 0)
    c.px(x0 + 5, base - 4, 0)
    rect(c, x0 + 1, base - 11, x0 + 6, base - 8)                                           # pot
    for dx, dy in ((2, -13), (3, -15), (5, -14)):
        c.px(x0 + dx, base + dy)
    c.line(x0 + 10, base - 13, x0 + 22, base - 13)                                         # shelf with jars
    for xx in (x0 + 11, x0 + 14, x0 + 17, x0 + 20):
        c.line(xx, base - 14, xx, base - 16)
        c.px(xx + 1, base - 14)
    c.line(x0 + 12, base - 7, x0 + 23, base - 7)                                           # table
    c.line(x0 + 13, base - 7, x0 + 13, base - 1)
    c.line(x0 + 22, base - 7, x0 + 22, base - 1)
    rect(c, x0 + 15, base - 10, x0 + 17, base - 8)                                         # mug
    c.px(x0 + 18, base - 9)
    c.disc(x0 + 20.5, base - 9, 1.2)                                                       # apple on the table



def living(c, cx, R, f2, base):
    """A fire, a rug and a hanging lamp."""
    fx = R - 12
    c.fill([(fx, base - 12), (fx + 9, base - 12), (fx + 9, base - 1), (fx, base - 1)])     # fireplace
    c.fill([(fx + 2, base - 8), (fx + 7, base - 8), (fx + 7, base - 1), (fx + 2, base - 1)], 0)
    for dx, dy in ((4, -2), (5, -2), (4, -3), (5, -4), (3, -2), (6, -3), (4, -5)):
        c.px(fx + dx, base + dy)
    c.line(fx - 2, base - 13, fx + 10, base - 13)                                          # mantel
    rect(c, fx + 3, base - 17, fx + 6, base - 14)                                          # clock
    for xx in range(cx + 3, fx - 3):                                                       # rug
        if xx % 2 == 0:
            c.px(xx, base - 1)
    c.line(cx + 2, base - 16, cx + 8, base - 16)                                           # hanging lamp
    c.line(cx + 5, f2 + 2, cx + 5, base - 16)
    c.px(cx + 5, base - 15)



def house_years(out):
    """Concept: the house grows a room at a time from year 5 to year 10. Some days everyone is indoors, some days out."""
    gy = T.ground_y
    CX, BASE = 62, 68

    def year(level):
        def draw(c):
            r = big_house(c, CX, BASE, level)
            if level == 1:      # at home in the one room
                stamp(c, MINI, r["kitchen"][0] - 6, r["kitchen"][1])
            elif level == 2:    # outside, waving
                gnome(c, 14, gy(14), "wave", 1)
            elif level == 3:    # raking outside, cat asleep on the bed
                gnome(c, 14, gy(14), "rake", 1)
                stamp(c, MINI_CAT, r["bed"][0] + 3, r["bed"][1])
            elif level == 4:    # both indoors on a rainy day, cat out of the rain by the fire
                stamp(c, MINI, r["study"][0] + 1, r["study"][1])
                stamp(c, MINI_PARTNER, r["kitchen"][0] - 4, r["kitchen"][1])
                stamp(c, MINI_CAT, r["living"][0] + 2, r["living"][1])
            elif level == 5:    # he reads on the grass with the pets, she is at the telescope
                gnome(c, 14, gy(14), "read", 1)
                stamp(c, CAT, 104, gy(104))
                stamp(c, DOG, 116, gy(116) + 1)
                stamp(c, MINI_PARTNER, r["attic"][0], r["attic"][1])
            else:               # everyone out together
                gnome(c, 12, gy(12), "cheer", 1)
                gnome(c, 33, gy(33), "stand", -1, partner=True)
                stamp(c, MINI_CAT, r["deck"][0], r["deck"][1] - 7)
                stamp(c, DOG, 115, gy(115) + 1)
        return draw

    panels = [scene(20, sky, year(lv)) for lv, sky in ((1, 1), (2, 4), (3, 6), (4, 8), (5, 0), (6, 12))]
    pad = 6
    sheet = Image.new("1", (3 * (W + pad) - pad, 2 * (H + pad) - pad), 1)
    for k, im in enumerate(panels):
        sheet.paste(im, ((k % 3) * (W + pad), (k // 3) * (H + pad)))
    sheet.convert("L").resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(os.path.join(out, "house-grows-v2@4x.png"))


def frame(c, x0, y0, x1, y1):
    """Paper rectangle with a fine outline (windows, pictures, cupboards)."""
    white(c, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)])


def zoom_house(c):
    """Close-up for the later years: the house fills the screen and the rooms are big enough for the gnomes."""
    L, R, MID = 8, 113, 60
    F1, F2, TOP = 111, 72, 33            # lower floor, upper floor, underside of the roof
    white(c, [(L, F1), (R, F1), (R, TOP), (L, TOP)])
    # roof, cut off by the top of the screen, with an attic window
    c.fill([(0, TOP + 1), (W - 1, TOP + 1), (W - 1, TOP - 4), (96, 0), (20, 0), (0, TOP - 6)])
    for row, yy in enumerate(range(TOP - 3, 0, -4)):
        for xx in range(2, W - 2):
            if (xx + row * 3) % 6 == 0 and c.a[yy, xx - 2] and c.a[yy, xx + 2] and c.a[yy - 2, xx]:
                c.px(xx, yy, 0)
                c.px(xx + 1, yy, 0)
    c.disc(58, 15, 9, 0)
    c.disc(58, 15, 8)
    c.disc(58, 15, 6.5, 0)
    c.line(58, 9, 58, 21)
    c.line(52, 15, 64, 15)
    # floors and walls
    for yy in (F1, F2, TOP):
        c.fill([(L - 2, yy), (R + 2, yy), (R + 2, yy + 2), (L - 2, yy + 2)])
    for xx in (L, L + 1, R - 1, R):
        c.line(xx, TOP, xx, F1)
    c.line(MID, TOP + 3, MID, F2 - 26)
    c.line(MID + 1, TOP + 3, MID + 1, F2 - 26)
    c.line(MID, F2 + 3, MID, F1 - 26)
    c.line(MID + 1, F2 + 3, MID + 1, F1 - 26)

    # ---- kitchen (lower left)
    x0, F = L + 3, F1 - 1
    c.fill([(x0, F - 15), (x0 + 15, F - 15), (x0 + 15, F), (x0, F)])                 # stove
    frame(c, x0 + 2, F - 10, x0 + 13, F - 3)
    c.line(x0 + 4, F - 8, x0 + 11, F - 8)
    c.line(x0 + 1, F - 16, x0 + 14, F - 16)
    c.line(x0 + 3, F - 17, x0 + 3, F2 + 3)                                            # stove pipe
    c.line(x0 + 4, F - 17, x0 + 4, F2 + 3)
    frame(c, x0 + 7, F - 22, x0 + 14, F - 17)                                         # pot
    c.line(x0 + 6, F - 22, x0 + 15, F - 22)
    c.px(x0 + 10, F - 23)
    for dx, dy in ((9, -26), (10, -28), (12, -29), (12, -32), (10, -33)):
        c.px(x0 + dx, F + dy)
    frame(c, x0 + 20, F - 33, x0 + 34, F - 19)                                        # window with curtains
    c.line(x0 + 27, F - 33, x0 + 27, F - 19)
    c.line(x0 + 20, F - 26, x0 + 34, F - 26)
    c.line(x0 + 20, F - 33, x0 + 23, F - 27)
    c.line(x0 + 34, F - 33, x0 + 31, F - 27)
    c.line(x0 + 18, F - 34, x0 + 36, F - 34)
    c.line(x0 + 38, F - 31, x0 + 46, F - 31)                                          # shelf with jars
    for xx in (x0 + 39, x0 + 42, x0 + 45):
        frame(c, xx - 1, F - 35, xx + 1, F - 32)
    c.fill([(x0 + 22, F - 12), (x0 + 44, F - 12), (x0 + 44, F - 11), (x0 + 22, F - 11)])  # table
    c.line(x0 + 24, F - 11, x0 + 24, F)
    c.line(x0 + 42, F - 11, x0 + 42, F)
    frame(c, x0 + 26, F - 17, x0 + 31, F - 13)                                        # teapot
    c.line(x0 + 31, F - 15, x0 + 33, F - 17)
    c.px(x0 + 28, F - 18)
    frame(c, x0 + 35, F - 15, x0 + 38, F - 13)                                        # mug
    c.disc(x0 + 41, F - 14, 1.5)                                                      # apple

    # ---- living room (lower right)
    x0 = MID + 3
    fx = R - 19
    c.fill([(fx, F - 22), (fx + 16, F - 22), (fx + 16, F), (fx, F)])                  # fireplace
    for yy in range(F - 20, F, 4):                                                    # stonework
        for xx in range(fx + 1, fx + 16, 5):
            c.px(xx + (yy // 4 % 2) * 2, yy, 0)
    c.fill([(fx + 3, F), (fx + 3, F - 10), (fx + 5, F - 13), (fx + 11, F - 13), (fx + 13, F - 10), (fx + 13, F)], 0)
    for dx, dy in ((7, -1), (8, -1), (9, -1), (8, -2), (7, -3), (9, -3), (8, -4), (8, -6), (6, -2), (10, -2), (9, -5)):
        c.px(fx + dx, F + dy)
    c.fill([(fx - 2, F - 24), (fx + 18, F - 24), (fx + 18, F - 23), (fx - 2, F - 23)])    # mantel
    c.disc(fx + 8, F - 28, 3)                                                         # clock
    c.disc(fx + 8, F - 28, 2, 0)
    c.px(fx + 8, F - 28)
    c.px(fx + 8, F - 29)
    c.line(fx + 2, F - 25, fx + 2, F - 28)                                            # candle
    c.px(fx + 2, F - 30)
    frame(c, x0 + 3, F - 34, x0 + 15, F - 25)                                         # picture of the tree
    c.line(x0 + 9, F - 27, x0 + 9, F - 30)
    c.disc(x0 + 9, F - 31, 2)
    c.fill([(x0 + 2, F - 17), (x0 + 4, F - 17), (x0 + 4, F), (x0 + 2, F)])            # armchair
    frame(c, x0 + 4, F - 9, x0 + 14, F - 3)
    frame(c, x0 + 12, F - 12, x0 + 15, F - 3)
    c.line(x0 + 5, F - 2, x0 + 5, F)
    c.line(x0 + 13, F - 2, x0 + 13, F)
    for xx in range(x0 + 16, fx - 2):                                                 # rug
        if xx % 3:
            c.px(xx, F)
    # ---- bedroom (upper left)
    x0, F = L + 3, F2 - 1
    c.fill([(x0, F - 20), (x0 + 2, F - 20), (x0 + 2, F), (x0, F)])                    # headboard
    c.disc(x0 + 1, F - 21, 1.5)
    frame(c, x0 + 2, F - 11, x0 + 34, F - 4)                                          # mattress and quilt
    for yy in range(F - 10, F - 4, 2):
        for xx in range(x0 + 13, x0 + 34, 2):
            c.px(xx + (yy // 2 % 2), yy)
    frame(c, x0 + 4, F - 14, x0 + 11, F - 11)                                         # pillow
    c.fill([(x0 + 34, F - 14), (x0 + 35, F - 14), (x0 + 35, F), (x0 + 34, F)])        # footboard
    c.line(x0 + 3, F - 3, x0 + 3, F)
    frame(c, x0 + 39, F - 9, x0 + 46, F)                                              # night stand
    c.px(x0 + 43, F - 5)
    c.line(x0 + 42, F - 10, x0 + 42, F - 14)                                          # candle
    c.px(x0 + 42, F - 16)
    c.disc(x0 + 26, F - 26, 6)                                                        # round window with the moon
    c.disc(x0 + 26, F - 26, 5, 0)
    c.disc(x0 + 27, F - 27, 2)
    c.disc(x0 + 28, F - 28, 1.6, 0)
    # ---- study (upper right)
    x0 = MID + 3
    bx = R - 20
    frame(c, bx, F - 34, R - 3, F)
    rng = random.Random(9)
    for shelf in (F - 26, F - 17, F - 8, F):
        c.line(bx, shelf, R - 3, shelf)
        for xx in range(bx + 2, R - 4):
            if rng.random() < 0.8:
                c.line(xx, shelf - 1, xx, shelf - rng.choice((4, 5, 6, 6)))
    c.fill([(x0 + 2, F - 11), (x0 + 22, F - 11), (x0 + 22, F - 10), (x0 + 2, F - 10)])    # desk
    c.line(x0 + 3, F - 10, x0 + 3, F)
    c.line(x0 + 21, F - 10, x0 + 21, F)
    frame(c, x0 + 5, F - 13, x0 + 12, F - 12)                                         # open book
    c.line(x0 + 17, F - 12, x0 + 17, F - 20)                                          # desk lamp
    c.fill([(x0 + 14, F - 20), (x0 + 20, F - 20), (x0 + 18, F - 24), (x0 + 16, F - 24)])
    c.disc(x0 + 9, F - 28, 4)                                                         # globe on the wall shelf
    c.disc(x0 + 9, F - 28, 3, 0)
    c.line(x0 + 6, F - 28, x0 + 12, F - 28)
    c.line(x0 + 4, F - 23, x0 + 14, F - 23)
    # leaves peeking in at the sides
    for x, y, a in ((3, 60, -70), (4, 84, -110), (2, 100, -60), (118, 52, -100), (117, 78, -70), (119, 98, -120)):
        c.a[T.leaf_mask(x, y, a, 7, 0.5)] = 1
    return {"kitchen": (38, F1 - 1), "living": (82, F1 - 1), "bed": (30, F2 - 12), "study": (76, F2 - 1), "hearth": (88, F1 - 1)}


def zoom_sheet(out):
    def home(c):       # both downstairs, cat on the bed, dog by the fire
        r = zoom_house(c)
        gnome(c, r["living"][0] - 1, r["living"][1], "sit", 1)
        gnome(c, r["kitchen"][0] + 5, r["kitchen"][1], "stand", -1, partner=True)
        stamp(c, CAT, r["bed"][0], r["bed"][1])
        stamp(c, DOG, r["study"][0] + 10, r["study"][1] + 0)

    def night(c):      # he reads upstairs, she naps by the fire
        r = zoom_house(c)
        gnome(c, r["study"][0] + 8, r["study"][1], "read", -1)
        gnome(c, r["living"][0] - 1, r["living"][1], "nap", 1, partner=True)
        stamp(c, CAT, r["kitchen"][0] + 4, r["kitchen"][1])
        stamp(c, DOG, r["bed"][0] + 14, r["bed"][1] + 11)

    ims = []
    for fn in (home, night):
        c = T.Canvas()
        fn(c)
        ims.append(c.image())
    pad = 6
    sheet = Image.new("1", (2 * (W + pad) - pad, H), 1)
    for k, im in enumerate(ims):
        sheet.paste(im, (k * (W + pad), 0))
    sheet.convert("L").resize((sheet.width * 5, sheet.height * 5), Image.NEAREST).save(os.path.join(out, "house-zoomed@5x.png"))


def main():
    write_art(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if len(sys.argv) < 2:
        return
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    house_years(out)
    zoom_sheet(out)
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
    names = ["stand", "wave", "water", "rake", "read", "sit", "nap", "lantern", "umbrella", "cheer"]
    cw, ch = 62, 60
    sh_ = Image.new("1", (cw * 5, ch * 2), 1)
    for k, name in enumerate(names):
        c = T.Canvas()
        gnome(c, 22, 56, name, 1)
        sh_.paste(c.image().crop((0, 0, cw, ch)), ((k % 5) * cw, (k // 5) * ch))
    sh_.convert("L").resize((sh_.width * 5, sh_.height * 5), Image.NEAREST).save(os.path.join(out, "gnome-activities@5x.png"))
    # close-up of the standing gnome
    c = T.Canvas()
    gnome(c, 18, 46, "stand", 1)
    gnome(c, 50, 46, "water", 1)
    gnome(c, 96, 46, "cheer", -1)
    gnome(c, 20, 100, "sit", 1)
    c.image().crop((0, 0, 122, 104)).convert("L").resize((122 * 6, 104 * 6), Image.NEAREST).save(os.path.join(out, "gnome-closeup.png"))


if __name__ == "__main__":
    main()
