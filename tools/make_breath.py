"""Breathing circle for the calm screen (hold down): five sizes, small to full, pen and ink.

  python tools/make_breath.py            -> art/breath/frame-0..4.png
  python tools/make_breath.py --style X  -> try another look (hatch, ripple)
"""
import argparse, math, os
from PIL import Image, ImageDraw

W, H, S = 122, 80, 4
R = [9, 15, 21, 28, 35]   # radius per frame: breathe in 0 -> 4, out 4 -> 0
CX, CY = 61, 40


def P(*a): return [v * S for v in a]


def frame(k, style):
    im = Image.new('L', (W * S, H * S), 255)
    d = ImageDraw.Draw(im)
    r = R[k]
    # the guide: a dotted ring at full size, so you can see how far there is to go
    for i in range(72):
        a = i * 2 * math.pi / 72
        x, y = CX + (R[-1] + 3.5) * math.cos(a), CY + (R[-1] + 3.5) * math.sin(a)
        d.ellipse(P(x - .55, y - .55, x + .55, y + .55), fill=0)
    if style == 'ripple':
        d.ellipse(P(CX - 3, CY - 3, CX + 3, CY + 3), fill=0)
        for j in range(k + 1):
            rr = R[j]
            d.ellipse(P(CX - rr, CY - rr, CX + rr, CY + rr), outline=0, width=int((1.6 if j == k else 1.0) * S))
    else:  # hatch: an inked ball, shaded with fine diagonal strokes, a lit crescent left clear
        mask = Image.new('L', im.size, 0)
        md = ImageDraw.Draw(mask)
        md.ellipse(P(CX - r, CY - r, CX + r, CY + r), fill=255)
        # light from the upper left: leave a crescent unshaded
        lx, ly, lr = CX - r * .32, CY - r * .32, r * .78
        md.ellipse(P(lx - lr, ly - lr, lx + lr, ly + lr), fill=0)
        hat = Image.new('L', im.size, 255)
        hd = ImageDraw.Draw(hat)
        step = 3.0
        t = -H
        while t < W + H:
            hd.line(P(t, 0, t + H, H), fill=0, width=int(0.9 * S))
            t += step
        im.paste(hat, (0, 0), mask)
        d.ellipse(P(CX - r, CY - r, CX + r, CY + r), outline=0, width=int(1.5 * S))
    return im.resize((W, H), Image.LANCZOS).point(lambda v: 0 if v < 140 else 255)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--style', default='hatch')
    ap.add_argument('--out', default=os.path.join(os.path.dirname(__file__), '..', 'art', 'breath'))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    for k in range(len(R)):
        frame(k, a.style).save(os.path.join(a.out, f'frame-{k}.png'))


if __name__ == '__main__':
    main()
