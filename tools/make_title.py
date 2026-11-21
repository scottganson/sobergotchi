#!/usr/bin/env python3
"""Landing page title (art/icons/title.png): "Sobergotchi" in Pirata One (Scott's pick, 2026-11-20), drawn large
and brought down to size so the strokes stay solid on the e-paper. Fitted to the frame width.
Usage: python tools/make_title.py [font file in fonts/]
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H, S = 116, 40, 8
TEXT = "Sobergotchi"
FONT = "PirataOne-Regular.ttf"


def make(font_path, cut=160, max_w=110, max_h=34):
    size = 60
    while True:  # largest size that fits the box
        font = ImageFont.truetype(font_path, size * S)
        l, t, r, b = ImageDraw.Draw(Image.new("L", (1, 1))).textbbox((0, 0), TEXT, font=font)
        if (r - l) <= max_w * S and (b - t) <= max_h * S:
            break
        size -= 1
    big = Image.new("L", (W * S, H * S), 255)
    ImageDraw.Draw(big).text(((W * S - (r - l)) / 2 - l, (H * S - (b - t)) / 2 - t), TEXT, font=font, fill=0)
    return big.resize((W, H), Image.BOX).point(lambda v: 0 if v < cut else 255).convert("1")


if __name__ == "__main__":
    name = sys.argv[1] if len(sys.argv) > 1 else FONT
    make(os.path.join(ROOT, "fonts", name)).save(os.path.join(ROOT, "art", "icons", "title.png"))
