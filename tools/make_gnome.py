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
    "      ###  ",
    "    ##...# ",
    "   ###.#..#",
    "   ###...##",
    "    #...#  ",
    "    #...#  ",
    "   #....#  ",
    "#  #....#  ",
    " # #....#  ",
    " ##.....#  ",
    "  #.#.#.#  ",
    "  ### ###  ",
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
    """Little cabin in the crown with its floor at (x, y), and a ladder to the ground."""
    white(c, [(x - 10, y), (x + 10, y), (x + 10, y - 11), (x - 10, y - 11)])
    c.fill([(x - 13, y - 11), (x + 13, y - 11), (x, y - 21)])            # roof
    for k in range(-9, 10, 3):                                           # shingle line
        c.px(x + k, y - 13, 0)
    c.fill([(x - 12, y), (x + 12, y), (x + 12, y + 1), (x - 12, y + 1)])  # floor beam
    white(c, [(x - 7, y), (x - 7, y - 8), (x - 2, y - 8), (x - 2, y)])    # door
    c.px(x - 3, y - 4)
    white(c, [(x + 2, y - 8), (x + 8, y - 8), (x + 8, y - 3), (x + 2, y - 3)])  # window
    c.line(x + 5, y - 8, x + 5, y - 3)
    c.line(x + 2, y - 5.5, x + 8, y - 5.5)
    c.line(x + 9, y - 18, x + 9, y - 23)                                  # chimney pipe
    for k in range(3):
        c.px(x + 10 + k, y - 25 - k * 2)
    gy = int(T.ground_y(x + 12))
    lx = x + 14                                                           # ladder
    c.line(lx, y + 1, lx, gy)
    c.line(lx + 4, y + 1, lx + 4, gy)
    for yy in range(int(y) + 4, gy, 4):
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


def later_years(out):
    """Concept sketches for years 5 to 10: tree house, bird, pets and a partner."""
    gy = T.ground_y

    def y5(c):
        tree_house(c, 50, 62)
        gnome(c, 24, gy(24), "wave", 1)

    def y7(c):
        tree_house(c, 50, 62)
        gnome(c, 26, gy(26), "read", 1)
        stamp(c, BIRD, 27, gy(26) - 37)
        stamp(c, CAT, 8, gy(8))
        stamp(c, DOG, 110, gy(110))

    def y8(c):
        tree_house(c, 50, 62)
        gnome(c, 14, gy(14), "stand", 1)
        stamp(c, BIRD, 15, gy(14) - 39)
        gnome(c, 36, gy(36), "wave", -1, partner=True)

    def y10(c):
        tree_house(c, 50, 62)
        gnome(c, 13, gy(13), "cheer", 1)
        gnome(c, 36, gy(36), "stand", -1, partner=True)
        stamp(c, BIRD, 36, gy(36) - 36)
        stamp(c, CAT, 51, gy(51))

    panels = [scene(20, 1, y5), scene(22, 4, y7), scene(23, 0, y8), scene(25, 12, y10)]
    pad = 6
    sheet = Image.new("1", (len(panels) * (W + pad) - pad, H), 1)
    for k, im in enumerate(panels):
        sheet.paste(im, (k * (W + pad), 0))
    sheet.convert("L").resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(os.path.join(out, "later-years@4x.png"))


def main():
    write_art(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if len(sys.argv) < 2:
        return
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    later_years(out)
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
