#!/usr/bin/env python3
"""Landing page title (art/icons/title.png): "Sobergotchi" in Great Vibes script, drawn large and brought down
to size with a slightly heavier pen so the thin hairlines stay joined on the e-paper (the 1-bit font left holes).
Usage: python tools/make_title.py [stroke] [cut]
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H, S = 116, 40, 8
TEXT = "Sobergotchi"


def make(stroke=0, cut=215, size=29):
    font = ImageFont.truetype(os.path.join(ROOT, "fonts", "GreatVibes-Regular.ttf"), size * S)
    big = Image.new("L", (W * S, H * S), 255)
    d = ImageDraw.Draw(big)
    l, t, r, b = d.textbbox((0, 0), TEXT, font=font, stroke_width=stroke)
    d.text(((W * S - (r - l)) / 2 - l, (H * S - (b - t)) / 2 - t), TEXT, font=font, fill=0,
           stroke_width=stroke, stroke_fill=0)
    small = big.resize((W, H), Image.BOX)  # average coverage per pixel
    return small.point(lambda v: 0 if v < cut else 255).convert("1")


if __name__ == "__main__":
    st = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    cut = int(sys.argv[2]) if len(sys.argv) > 2 else 215
    make(st, cut).save(os.path.join(ROOT, "art", "icons", "title.png"))
