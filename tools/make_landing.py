#!/usr/bin/env python3
"""Landing page picture (art/icons/landing.png): the young tree on a clear morning, Pip waving, birds overhead.

Built from the existing pen-and-ink layers (art/sky, art/tree + treemask, art/gnome + gnomemask) so it matches
the home screen, cropped to fit inside the landing page frame.
Usage: python tools/make_landing.py
"""
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKY, TREE, POSE = "sky-04-birds", "stage-16", "pose-01-wave"
TOP = 16  # room for the flourish above the scene
CROP = 3  # trim each side so the picture sits inside the page frame


def layer(folder, name):
    return Image.open(os.path.join(ROOT, "art", folder, name + ".png")).convert("L")


def main():
    pic = layer("sky", SKY)
    for art, mask, name in (("tree", "treemask", TREE), ("gnome", "gnomemask", POSE)):
        m = layer(mask, name).point(lambda v: 255 - v)  # black in the mask = paper behind the drawing
        pic.paste(255, (0, 0), m)
        pic.paste(layer(art, name), (0, 0), layer(art, name).point(lambda v: 255 - v))
    pic = pic.crop((CROP, 0, pic.width - CROP, pic.height))
    # a pen flourish above the scene: a fine line either side of a small diamond, dots at the ends
    full = Image.new("L", (pic.width, pic.height + TOP), 255)
    full.paste(pic, (0, TOP))
    d = ImageDraw.Draw(full)
    cx, cy = full.width // 2, 6
    d.polygon([(cx, cy - 3), (cx + 3, cy), (cx, cy + 3), (cx - 3, cy)], outline=0)
    d.point((cx, cy), fill=0)
    for side in (-1, 1):
        d.line([(cx + side * 6, cy), (cx + side * 40, cy)], fill=0)
        d.point((cx + side * 44, cy), fill=0)
        d.point((cx + side * 47, cy), fill=0)
    pic = full.point(lambda v: 0 if v < 128 else 255).convert("1")
    pic.save(os.path.join(ROOT, "art", "icons", "landing.png"))


if __name__ == "__main__":
    main()
