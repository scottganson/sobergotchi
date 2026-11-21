#!/usr/bin/env python3
"""Landing page picture (art/icons/landing.png), built from the existing pen-and-ink layers (art/sky, art/tree +
treemask, art/gnome + gnomemask, art/world + worldmask, art/gnomebig) so it matches the rest of the device,
cropped to fit inside the landing page frame, with a pen flourish above.

A scene is a sky name (or None for plain paper) and a list of layers drawn over it, in order.
Usage: python tools/make_landing.py            (writes the chosen SCENE)
       python tools/make_landing.py --samples  (a sheet of the SAMPLES for choosing)
"""
import os
import sys
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOP = 16  # room for the flourish above the scene
CROP = 3  # trim each side so the picture sits inside the page frame

SAMPLES = [  # (label, sky, [(folder, name)])
    ("Sapling, clear sky", "sky-04-birds", [("tree", "stage-16"), ("gnome", "pose-01-wave")]),
    ("Sunrise sprout", "sky-02-morning", [("tree", "stage-08"), ("gnome", "pose-02-water")]),
    ("Fair day, raking", "sky-01-fair", [("tree", "stage-13"), ("gnome", "pose-03-rake")]),
    ("Reading under the tree", "sky-03-partly", [("tree", "stage-19"), ("gnome", "pose-04-read")]),
    ("Tea in the shade", "sky-00-clear", [("tree", "stage-18"), ("gnome", "pose-05-sit")]),
    ("Cheering, apples", "sky-04-birds", [("tree", "stage-19"), ("gnome", "pose-09-cheer")]),
    ("Lantern in the fog", "sky-11-fog", [("tree", "stage-15"), ("gnome", "pose-07-lantern")]),
    ("First snow", "sky-10-snow", [("tree", "stage-17"), ("gnome", "pose-08-umbrella")]),
    ("Tree house", "sky-04-birds", [("world", "world-03")]),
    ("Tree house and bridge", "sky-02-morning", [("world", "world-08")]),
    ("Big tree house", "sky-01-fair", [("world", "world-15")]),
    ("Pip up close", None, [("gnomebig", "pose-01-wave")]),
    # with the circle and triangle (Scott, 2026-11-20)
    ("Circle in the flourish", "sky-04-birds", [("tree", "stage-16"), ("gnome", "pose-01-wave")], "aa"),
    ("Circle for the sun", "sky-04-birds", [("aa", (104, 16, 14, True)), ("tree", "stage-16"), ("gnome", "pose-01-wave")]),
    ("Sapling inside the circle", None, [("aa", (61, 48, 42, False)), ("tree", "stage-08")]),
    ("Tree house + circle", "sky-01-fair", [("world", "world-15")], "aa"),
]
SCENE = SAMPLES[0]
MASK = {"tree": "treemask", "gnome": "gnomemask", "world": "worldmask"}


def layer(folder, name):
    return Image.open(os.path.join(ROOT, "art", folder, name + ".png")).convert("L")


def aa(img, cx, cy, r, clear):
    """The circle and triangle, drawn with a fine pen (4x supersampled). clear: paper under it first (over the sun)."""
    S = 4
    big = img.resize((img.width * S, img.height * S), Image.NEAREST)
    d = ImageDraw.Draw(big)
    if clear:
        d.rectangle([(cx - r - 3) * S, 0, (cx + r + 4) * S, (cy + r + 3) * S], fill=255)
    w = S + (S // 2 if r > 20 else 0)
    d.ellipse([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S], outline=0, width=w)
    import math
    pts = [((cx + r * math.cos(math.radians(a))) * S, (cy + r * math.sin(math.radians(a))) * S) for a in (-90, 30, 150)]
    d.line(pts + [pts[0]], fill=0, width=w, joint="curve")
    return big.resize(img.size, Image.BOX).point(lambda v: 0 if v < 140 else 255)


def scene(sky, layers, flourish="diamond"):
    pic = layer("sky", sky) if sky else Image.new("L", (122, 114), 255)
    for folder, name in layers:
        if folder == "aa":
            pic = aa(pic, *name)
            continue
        art = layer(folder, name)
        if art.size != pic.size:  # e.g. the 120x120 big gnome: centred, sitting on the bottom
            a = Image.new("L", pic.size, 255)
            a.paste(art.crop((0, max(0, art.height - pic.height), art.width, art.height)), ((pic.width - art.width) // 2, 0))
            art = a
        if folder in MASK:
            pic.paste(255, (0, 0), layer(MASK[folder], name).point(lambda v: 255 - v))  # black in mask = paper
        pic.paste(art, (0, 0), art.point(lambda v: 255 - v))
    pic = pic.crop((CROP, 0, pic.width - CROP, pic.height))
    # a pen flourish above the scene: a fine line either side of a small diamond, dots at the ends
    full = Image.new("L", (pic.width, pic.height + TOP), 255)
    full.paste(pic, (0, TOP))
    d = ImageDraw.Draw(full)
    cx, cy = full.width // 2, 6
    gap = 6
    if flourish == "aa":
        full = aa(full, cx, cy + 1, 6.5, False)
        d = ImageDraw.Draw(full)
        gap = 10
    else:
        d.polygon([(cx, cy - 3), (cx + 3, cy), (cx, cy + 3), (cx - 3, cy)], outline=0)
        d.point((cx, cy), fill=0)
    for side in (-1, 1):
        d.line([(cx + side * gap, cy), (cx + side * 40, cy)], fill=0)
        d.point((cx + side * 44, cy), fill=0)
        d.point((cx + side * 47, cy), fill=0)
    return full.point(lambda v: 0 if v < 128 else 255).convert("1")


def main():
    scene(*SCENE[1:]).save(os.path.join(ROOT, "art", "icons", "landing.png"))


if __name__ == "__main__":
    main()
