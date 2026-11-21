#!/usr/bin/env python3
"""Art for Your path: leaf and basket (Leaf catch), the nine Tree tiles, padlocks.

  art/icons/leaf.png, basket.png       Leaf catch
  art/tiles/tile-0..8.png              Tree tiles: a 108x108 crop of the tree over the fair sky, cut 3x3 (8 = the gap)
Run from the repo root: python tools/make_path_art.py
"""
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
A = lambda *p: os.path.join(ROOT, "art", *p)


def save_rows(rows, path):
    im = Image.new("L", (len(rows[0]), len(rows)), 255)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "#":
                im.putpixel((x, y), 0)
    im.save(path)


LEAF = [
    "......#......",
    ".....###.....",
    "..#..#.#..#..",
    "..##.#.#.##..",
    "...#######...",
    "#..#..#..#..#",
    "##.#.###.#.##",
    ".###..#..###.",
    "..#########..",
    "...#..#..#...",
    "....#####....",
    "......#......",
    "......#......",
]
BASKET = [
    "....##################....",
    "...#..................#...",
    "..#....................#..",
    "##########################",
    "#.#.#.#.#.#.#.#.#.#.#.#.##",
    "##########################",
    ".#.#.#.#.#.#.#.#.#.#.#.##.",
    ".########################.",
    "..#.#.#.#.#.#.#.#.#.#.##..",
    "..######################..",
    "...#.#.#.#.#.#.#.#.#.##...",
    "...####################...",
]

# tiny digits for the tile corners, 3x5
DIG = {"1": [".#.", "##.", ".#.", ".#.", "###"], "2": ["##.", "..#", ".#.", "#..", "###"],
       "3": ["##.", "..#", ".#.", "..#", "##."], "4": ["#.#", "#.#", "###", "..#", "..#"],
       "5": ["###", "#..", "##.", "..#", "##."], "6": [".##", "#..", "###", "#.#", "###"],
       "7": ["###", "..#", ".#.", ".#.", ".#."], "8": ["###", "#.#", "###", "#.#", "###"]}


def tiles():
    sky = Image.open(A("sky", "sky-01-fair.png")).convert("1")
    mask = Image.open(A("treemask", "stage-19.png")).convert("1")
    tree = Image.open(A("tree", "stage-19.png")).convert("1")
    W, H = sky.size
    pic = Image.new("1", (W, H), 1)
    for y in range(H):
        for x in range(W):
            v = sky.getpixel((x, y))
            if not mask.getpixel((x, y)):
                v = 1
            if not tree.getpixel((x, y)):
                v = 0
            pic.putpixel((x, y), v)
    x0, y0 = (W - 108) // 2, H - 108
    pic = pic.crop((x0, y0, x0 + 108, y0 + 108))
    os.makedirs(A("tiles"), exist_ok=True)
    for i in range(9):
        t = Image.new("L", (36, 36), 255)
        if i < 8:
            t.paste(pic.crop(((i % 3) * 36, (i // 3) * 36, (i % 3) * 36 + 36, (i // 3) * 36 + 36)).convert("L"), (0, 0))
            d = ImageDraw.Draw(t)
            d.rectangle([0, 0, 35, 35], outline=0)
            d.rectangle([28, 27, 34, 34], fill=255, outline=0)  # number badge, bottom right
            for yy, r in enumerate(DIG[str(i + 1)]):
                for xx, c in enumerate(r):
                    if c == "#":
                        t.putpixel((30 + xx, 29 + yy), 0)
        t.save(A("tiles", f"tile-{i}.png"))


if __name__ == "__main__":
    save_rows(LEAF, A("icons", "leaf.png"))
    save_rows(BASKET, A("icons", "basket.png"))
    tiles()
    print("ok")
