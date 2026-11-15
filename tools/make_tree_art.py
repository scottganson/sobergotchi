#!/usr/bin/env python3
"""Generate the 26 tree-stage drawings as SVG files in art/tree/.

The SVGs are the source art: you can open and redraw any of them in Inkscape,
Figma or Illustrator. Shapes are painted in order (later covers earlier); black
is ink, white is paper and greys become dither textures on the e-paper.
Re-running this script overwrites art/tree/*.svg.
"""
import math
import os
import random

W, H = 122, 114
GROUND = 94  # ground line height at the centre
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "art", "tree")

# Greys the build turns into dither textures
G12, G25, G50, G75 = "#e1e1e1", "#bfbfbf", "#7f7f7f", "#404040"
K, P = "#000", "#fff"

STAGES = [
    ("1-3 days", "Dry seed"),
    ("4-6 days", "Swollen seed"),
    ("7-11 days", "Sprout folded over"),
    ("12-14 days", "Sprout starting to stand"),
    ("15-29 days", "Sprout standing up"),
    ("1 month", "Branch 1, leaf 1"),
    ("2 months", "Leaf 2"),
    ("3 months", "Branch 2, leaf 3"),
    ("4 months", "Leaf 4"),
    ("5 months", "Leaf 5"),
    ("6 months", "Branch 3, leaf 6"),
    ("7 months", "Leaf 7"),
    ("8 months", "Leaf 8"),
    ("9 months", "Branch 4, leaf 9"),
    ("10 months", "Leaf 10"),
    ("11 months", "Leaf 11"),
    ("1 year", "Youth tree, 1 apple"),
    ("2 years", "Youth tree, 2 apples"),
    ("3 years", "Youth tree, 3 apples"),
    ("4 years", "Youth tree, 4 apples"),
    ("5 years", "Mature tree, 5 apples"),
    ("6 years", "Mature tree, 5 apples, 1 on the ground"),
    ("7 years", "Mature tree, 5 apples, 2 on the ground"),
    ("8 years", "Mature tree, 5 apples, 3 on the ground"),
    ("9 years", "Mature tree, 5 apples, 4 on the ground"),
    ("10 years", "Mature tree, 5 apples, 5 on the ground"),
]


def f(v):
    return f"{v:.2f}".rstrip("0").rstrip(".")


def pts(points, close=False):
    return "M" + " L".join(f"{f(x)},{f(y)}" for x, y in points) + (" Z" if close else "")


def ground_y(x):
    """Gentle mound: highest in the middle."""
    t = (x - W / 2) / (W / 2)
    return GROUND + 4.5 * t * t


class Svg:
    def __init__(self):
        self.out = []
        self.n = 0

    def add(self, s):
        self.out.append(s)

    def path(self, d, fill="none", stroke=None, w=1.4, extra=""):
        st = f' stroke="{stroke}" stroke-width="{f(w)}"' if stroke else ""
        self.add(f'<path d="{d}" fill="{fill}"{st}{extra}/>')

    def line(self, x0, y0, x1, y1, w=1.4, stroke=K):
        self.path(f"M{f(x0)},{f(y0)} L{f(x1)},{f(y1)}", stroke=stroke, w=w)

    def circle(self, x, y, r, fill="none", stroke=None, w=1.4):
        st = f' stroke="{stroke}" stroke-width="{f(w)}"' if stroke else ""
        self.add(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r)}" fill="{fill}"{st}/>')

    def ellipse(self, x, y, rx, ry, fill="none", stroke=None, w=1.4, rot=0, extra=""):
        st = f' stroke="{stroke}" stroke-width="{f(w)}"' if stroke else ""
        tr = f' transform="rotate({f(rot)} {f(x)} {f(y)})"' if rot else ""
        self.add(f'<ellipse cx="{f(x)}" cy="{f(y)}" rx="{f(rx)}" ry="{f(ry)}" fill="{fill}"{st}{tr}{extra}/>')

    def uid(self):
        self.n += 1
        return f"c{self.n}"

    def doc(self, title):
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">\n'
                f'<title>{title}</title>\n'
                f'<g stroke-linecap="round" stroke-linejoin="round">\n' + "\n".join(self.out) + '\n</g>\n</svg>\n')


# ---------------------------------------------------------------- scenery

def sky(s):
    sx, sy, r = 101, 16, 6.5
    for k in range(12):
        a = k * math.pi / 6 + 0.26
        r0, r1 = r + 2.8, r + (6.4 if k % 2 == 0 else 4.6)
        s.line(sx + math.cos(a) * r0, sy + math.sin(a) * r0, sx + math.cos(a) * r1, sy + math.sin(a) * r1, 1.3)
    s.circle(sx, sy, r, fill=P, stroke=K, w=1.5)
    # cloud: puffs with a flat bottom
    puffs = [(13.5, 24, 5), (21, 19.5, 7), (30, 20.5, 6), (36.5, 24.5, 4.5)]
    cid = s.uid()
    s.add(f'<clipPath id="{cid}"><rect x="0" y="0" width="{W}" height="29"/></clipPath>')
    s.add(f'<g clip-path="url(#{cid})">')
    for x, y, r in puffs:
        s.circle(x, y, r, fill=K)
    s.add('</g>')
    cid2 = s.uid()
    s.add(f'<clipPath id="{cid2}"><rect x="0" y="0" width="{W}" height="27.6"/></clipPath>')
    s.add(f'<g clip-path="url(#{cid2})">')
    for x, y, r in puffs:
        s.circle(x, y, r - 1.4, fill=P)
    s.add(f'<rect x="12" y="22" width="26" height="6" fill="{P}"/>')
    s.add('</g>')


def hills(s):
    far = "M-2,90 Q14,72 34,79 Q48,84 58,92 L58,110 L-2,110 Z"
    near = "M52,94 Q78,78 100,81 Q114,83 124,88 L124,110 L52,110 Z"
    s.path(far, fill=P, stroke=K, w=1.1)
    s.path(near, fill=G12, stroke=K, w=1.1)


def flower(s, x, big=False):
    gy = ground_y(x)
    h = 7.5 if big else 6
    s.path(f"M{f(x)},{f(gy)} q0.8,{f(-h / 2)} 0,{f(-h)}", stroke=K, w=1.1)
    s.circle(x, gy - h - 1, 2.3, fill=P, stroke=K, w=1.1)
    s.circle(x, gy - h - 1, 0.7, fill=K)


def ground(s, rng, stage, flowers=()):
    top = [(x, ground_y(x)) for x in range(-2, W + 3, 2)]
    s.path(pts(top + [(W + 2, H + 2), (-2, H + 2)], True), fill=G25)
    # darker band of deep soil
    deep = [(x, H - 4.5 + 1.2 * math.sin(x / 7.0)) for x in range(-2, W + 3, 3)]
    s.path(pts(deep + [(W + 2, H + 2), (-2, H + 2)], True), fill=G50)
    # stones
    for x, dy, rx in ((9, 9, 2.6), (27, 13, 1.9), (44, 8.5, 1.6), (83, 12, 2.2), (99, 8, 1.7), (113, 12.5, 2.4)):
        s.ellipse(x, ground_y(x) + dy, rx, rx * 0.68, fill=P, stroke=K, w=1.0)
    # grass: an uneven fringe plus a few taller tufts
    x = 1.0
    while x < W:
        if abs(x - W / 2) > 6:
            gy = ground_y(x)
            h = rng.uniform(1.8, 4.2)
            lean = rng.uniform(-1.4, 1.4)
            s.path(f"M{f(x)},{f(gy)} q{f(lean * 0.2)},{f(-h * 0.6)} {f(lean)},{f(-h)}", stroke=K, w=1.1)
        x += rng.uniform(2.2, 3.6)
    for x in (7, 22, 37, 86, 103, 116):
        gy = ground_y(x)
        for dx, h, lean in ((-2.2, 5, -2.4), (0, 7.5, 0.4), (2.2, 5.5, 2.6)):
            s.path(f"M{f(x + dx)},{f(gy)} q{f(lean * 0.2)},{f(-h * 0.6)} {f(lean)},{f(-h)}", stroke=K, w=1.2)
    for i, x in enumerate(flowers):
        flower(s, x, big=(i % 2 == 0))
    s.path(pts(top), stroke=K, w=2.0)


# ---------------------------------------------------------------- small parts

def seed(s, cx, cy, rx, ry, angle):
    s.ellipse(cx, cy, rx * 1.75, ry * 1.7, fill=P, stroke=K, w=1.0, extra=' stroke-dasharray="1.5 1.9"')
    d = (f"M{f(cx - rx)},{f(cy)} Q{f(cx - rx * 0.6)},{f(cy - ry * 1.25)} {f(cx + rx * 0.35)},{f(cy - ry * 0.8)} "
         f"Q{f(cx + rx * 1.05)},{f(cy - ry * 0.3)} {f(cx + rx)},{f(cy + ry * 0.25)} "
         f"Q{f(cx + rx * 0.4)},{f(cy + ry * 1.2)} {f(cx - rx * 0.5)},{f(cy + ry * 0.85)} Q{f(cx - rx * 1.05)},{f(cy + ry * 0.5)} {f(cx - rx)},{f(cy)} Z")
    rot = f' transform="rotate({angle} {f(cx)} {f(cy)})"'
    s.path(d, fill=K, extra=rot)
    s.path(f"M{f(cx - rx * 0.55)},{f(cy - ry * 0.25)} Q{f(cx - rx * 0.3)},{f(cy - ry * 0.75)} {f(cx + rx * 0.2)},{f(cy - ry * 0.5)}",
           stroke=P, w=1.2, extra=rot)


def roots(s, x, y, depth, spread, rng, n=3):
    for i in range(n):
        ang = math.radians(90 + (i - (n - 1) / 2) * spread)
        L = depth * rng.uniform(0.8, 1.0)
        ex, ey = x + math.cos(ang) * L * 0.7, min(y + math.sin(ang) * L, H - 2.5)
        mx, my = x + math.cos(ang) * L * 0.15 + rng.uniform(-1.5, 1.5), y + L * 0.5
        d = f"M{f(x)},{f(y)} Q{f(mx)},{f(my)} {f(ex)},{f(ey)}"
        s.path(d, stroke=P, w=3.0)
        s.path(d, stroke=K, w=1.2)


def leaf(s, bx, by, angle_deg, length, fat=0.62, vein=True):
    """A two-tone leaf: white on the lit side, black underneath."""
    a = math.radians(angle_deg)
    ux, uy = math.cos(a), math.sin(a)
    nx, ny = -uy, ux
    if ny < 0:  # normal points down the page so the dark half is underneath
        nx, ny = -nx, -ny
    tx, ty = bx + ux * length, by + uy * length
    wdt = length * fat
    c1 = (bx + ux * length * 0.42 + nx * wdt, by + uy * length * 0.42 + ny * wdt)
    c2 = (bx + ux * length * 0.42 - nx * wdt, by + uy * length * 0.42 - ny * wdt)
    full = f"M{f(bx)},{f(by)} Q{f(c1[0])},{f(c1[1])} {f(tx)},{f(ty)} Q{f(c2[0])},{f(c2[1])} {f(bx)},{f(by)} Z"
    half = f"M{f(bx)},{f(by)} Q{f(c1[0])},{f(c1[1])} {f(tx)},{f(ty)} Z"
    s.path(full, fill=P, stroke=K, w=1.3)
    s.path(half, fill=K)
    if vein and length >= 10:
        for t in (0.38, 0.62):
            px, py = bx + ux * length * t, by + uy * length * t
            s.line(px, py, px - nx * wdt * 0.3 + ux * 1.6, py - ny * wdt * 0.3 + uy * 1.6, 0.9)


def stem_poly(s, centre, w0, w1):
    """Tapered solid stem along a list of centre points (bottom first)."""
    left, right = [], []
    n = len(centre)
    for i, (x, y) in enumerate(centre):
        w = w0 + (w1 - w0) * i / (n - 1)
        left.append((x - w / 2, y))
        right.append((x + w / 2, y))
    s.path(pts(left + right[::-1], True), fill=K, stroke=K, w=0.6)


def limb(s, x0, y0, x1, y1, w0, w1, bend=0.0):
    """Tapered curved branch from (x0, y0) to (x1, y1)."""
    mx, my = (x0 + x1) / 2 + bend, (y0 + y1) / 2 + abs(bend) * 0.8
    L = math.hypot(x1 - x0, y1 - y0) or 1
    nx, ny = -(y1 - y0) / L, (x1 - x0) / L
    d = (f"M{f(x0 + nx * w0 / 2)},{f(y0 + ny * w0 / 2)} Q{f(mx + nx * (w0 + w1) / 4)},{f(my + ny * (w0 + w1) / 4)} {f(x1 + nx * w1 / 2)},{f(y1 + ny * w1 / 2)} "
         f"L{f(x1 - nx * w1 / 2)},{f(y1 - ny * w1 / 2)} Q{f(mx - nx * (w0 + w1) / 4)},{f(my - ny * (w0 + w1) / 4)} {f(x0 - nx * w0 / 2)},{f(y0 - ny * w0 / 2)} Z")
    s.path(d, fill=K, stroke=K, w=0.5)


def apple(s, x, y, r=3.7, stem=True):
    """A black apple with a white rim so it stands out from foliage and grass."""
    stalk = f"M{f(x + 0.2)},{f(y - r * 0.8)} q0.2,-1.8 1.5,-2.9"
    if stem:
        s.path(stalk, stroke=P, w=3.0)
    s.ellipse(x, y, r * 1.06, r, fill=K, stroke=P, w=2.4)
    s.ellipse(x, y, r * 1.06, r, fill=K)
    s.path(f"M{f(x - 1.1)},{f(y - r + 0.2)} L{f(x + 1.1)},{f(y - r + 0.2)}", stroke=K, w=1.0)
    s.circle(x - r * 0.42, y - r * 0.32, 0.95, fill=P)
    if stem:
        s.path(stalk, stroke=K, w=1.2)
        s.path(f"M{f(x + 1.5)},{f(y - r - 2.6)} q2.4,-1.6 3.8,0.2 q-2.2,1.2 -3.8,-0.2 Z", fill=K, stroke=K, w=0.5)


def trunk(s, cx, base_y, top_y, w_base, w_top, lean=0.0):
    """Tapered trunk with flared roots; lit from the left."""
    hb, ht = w_base / 2, w_top / 2
    tx = cx + lean
    flare = w_base * 0.55
    d = (f"M{f(cx - hb - flare)},{f(base_y + 1.5)} Q{f(cx - hb - 0.5)},{f(base_y - 1)} {f(cx - hb + 0.3)},{f(base_y - 8)} "
         f"Q{f(cx - hb + 1)},{f((base_y + top_y) / 2)} {f(tx - ht)},{f(top_y)} L{f(tx + ht)},{f(top_y)} "
         f"Q{f(cx + hb - 1)},{f((base_y + top_y) / 2)} {f(cx + hb - 0.3)},{f(base_y - 8)} "
         f"Q{f(cx + hb + 0.5)},{f(base_y - 1)} {f(cx + hb + flare)},{f(base_y + 1.5)} Z")
    s.path(d, fill=P, stroke=K, w=1.6)
    cid = s.uid()
    s.add(f'<clipPath id="{cid}"><path d="{d}"/></clipPath>')
    # shadow side: a wavy-edged black band on the right
    edge = [(cx + lean * (base_y - y) / max(1.0, base_y - top_y) + w_base * 0.12 + 0.9 * math.sin(y / 2.6), y)
            for y in [top_y - 1 + i * 2.0 for i in range(int((base_y - top_y) / 2) + 3)]]
    band = edge + [(cx + w_base + flare, edge[-1][1]), (cx + w_base + flare, edge[0][1])]
    s.add(f'<g clip-path="url(#{cid})">')
    s.path(pts(band, True), fill=K)
    s.add('</g>')
    # bark flecks on the lit side
    for k in range(3):
        y0 = base_y - 7 - k * (base_y - top_y) / 3.6
        x0 = cx - hb * 0.45 + (k % 2) * 1.2 + lean * (base_y - y0) / max(1.0, base_y - top_y)
        s.line(x0, y0, x0 + 0.3, y0 - 3.2, 1.0)


def canopy(s, rng, cx, cy, A, B, bumps, bump_r, light=(-1.0, -1.0), marks=8):
    """A leafy crown: scalloped outline, three tones of shading, small leaf marks."""
    shapes = [f'<ellipse cx="{f(cx)}" cy="{f(cy)}" rx="{f(A)}" ry="{f(B)}"/>']
    lobes = []
    for k in range(bumps):
        a = math.tau * k / bumps + rng.uniform(-0.1, 0.1) - math.pi / 2
        r = bump_r * rng.uniform(0.85, 1.15)
        # flatter underside, fuller top
        x, y = cx + math.cos(a) * A * 0.96, cy + math.sin(a) * B * (0.96 if math.sin(a) < 0 else 0.86)
        lobes.append((x, y, r))
        shapes.append(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r)}"/>')
    body = "".join(shapes)
    cid = s.uid()
    s.add(f'<clipPath id="{cid}">{body}</clipPath>')
    s.add(f'<g fill="{K}" stroke="{K}" stroke-width="3.2">{body}</g>')
    s.add(f'<g fill="{P}">{body}</g>')
    k1, k2 = min(A, B) * 0.16, min(A, B) * 0.42
    s.add(f'<g clip-path="url(#{cid})">'
          f'<rect x="-10" y="-10" width="{W + 20}" height="{H + 20}" fill="{G50}"/>'
          f'<g fill="{G12}" transform="translate({f(light[0] * k1)},{f(light[1] * k1)})">{body}</g>'
          f'<g fill="{P}" transform="translate({f(light[0] * k2)},{f(light[1] * k2)})">{body}</g>'
          f'</g>')
    # clumps of foliage: short arcs that echo the scallops, kept off the darkest edge
    s.add(f'<g clip-path="url(#{cid})">')
    placed = []
    tries = 0
    while len(placed) < marks and tries < 400:
        tries += 1
        a = rng.uniform(0, math.tau)
        d = math.sqrt(rng.uniform(0.02, 0.8))
        x, y = cx + math.cos(a) * A * d - A * 0.08, cy + math.sin(a) * B * d - B * 0.08
        if any(math.hypot(x - px, y - py) < 9.5 for px, py in placed):
            continue
        placed.append((x, y))
        r = rng.uniform(3.4, 5.2)
        s.path(f"M{f(x - r)},{f(y)} A{f(r)},{f(r)} 0 0 0 {f(x + r)},{f(y)}", stroke=K, w=1.2)
        s.path(f"M{f(x + r)},{f(y)} a{f(r * 0.7)},{f(r * 0.7)} 0 0 0 {f(r * 1.2)},{f(-r * 0.5)}", stroke=K, w=1.2)
    s.add('</g>')
    return placed


# ---------------------------------------------------------------- stages

def sapling(s, month, rng):
    """month 1..11: a young stem that gains a branch or a leaf every month."""
    cx = W / 2
    gy = ground_y(cx)
    height = 30 + month * 4.3
    top = gy - height

    def stem_x(y):
        t = (gy - y) / height
        return cx + 2.4 * math.sin(t * math.pi * 1.15) - 1.2 * t

    centre = [(stem_x(gy + 1 - i * (height + 1) / 16), gy + 1 - i * (height + 1) / 16) for i in range(17)]
    stem_poly(s, centre, 3.0 + month * 0.22, 1.3)
    branches_at = [m for m in (1, 3, 6, 9) if m <= month]
    tips = []
    for k, m in enumerate(branches_at):
        side = -1 if k % 2 == 0 else 1
        by = gy - height * (0.34 + 0.14 * k)
        bx = stem_x(by)
        age = month - m
        L = 11 + age * 1.9
        ex, ey = bx + side * L * 0.9, by - L * 0.62
        limb(s, bx, by, ex, ey, 2.0 + age * 0.12, 1.1, bend=side * 1.2)
        tips.append((ex, ey, side, bx, by))
    # leaf places, in the order they appear
    spots = [None] * 11
    order = []
    for ex, ey, side, bx, by in tips:
        order.append((ex, ey, -90 + side * 48))
    crown = (stem_x(top) + 0.2, top + 1.5, -92)
    mids = [((ex + bx) / 2, (ey + by) / 2, -90 + side * 100) for ex, ey, side, bx, by in tips]
    stem_spots = []
    for k in range(4):
        y = gy - height * (0.5 + 0.13 * k)
        side = 1 if k % 2 == 0 else -1
        stem_spots.append((stem_x(y), y, -90 + side * 62))
    order = order + [crown] + mids + stem_spots
    size = 12.5 + min(month, 9) * 0.45
    for i in range(min(month, len(order))):
        x, y, ang = order[i]
        leaf(s, x, y, ang + rng.uniform(-7, 7), size * (1.0 if i < 5 else 0.9))


def youth_tree(s, year, rng):
    cx = W / 2 + 1
    gy = ground_y(cx)
    g = year  # 1..4
    A, B = 21 + g * 3.6, 17 + g * 2.6
    ccy = gy - 30 - g * 2.2 - B * 0.72
    tw = 8 + g * 1.2
    top_y = ccy + B * 0.55
    limb(s, cx - 1, top_y + 9, cx - A * 0.5, ccy + B * 0.3, 2.6, 1.6, bend=-1.5)
    limb(s, cx + 1, top_y + 8, cx + A * 0.55, ccy + B * 0.2, 2.6, 1.6, bend=1.5)
    trunk(s, cx, gy, top_y, tw, tw * 0.7, lean=0.6)
    canopy(s, rng, cx, ccy, A, B, bumps=9 + g, bump_r=7.2 + g * 0.5, marks=3 + g * 2)
    # apples hang from the underside of the crown
    hang = [(cx + A * 0.62, ccy + B * 0.98), (cx - A * 0.66, ccy + B * 0.9), (cx + A * 0.1, ccy + B * 1.12), (cx - A * 1.02, ccy + B * 0.32)]
    for x, y in hang[:year]:
        apple(s, x, y + 3)


def mature_tree(s, ground_apples, rng):
    cx = W / 2 + 1
    gy = ground_y(cx)
    A, B = 44, 27
    ccy = 36
    top_y = ccy + B * 0.6
    limb(s, cx - 2, top_y + 12, cx - 26, ccy + 14, 3.6, 2.0, bend=-2)
    limb(s, cx + 2, top_y + 11, cx + 27, ccy + 12, 3.6, 2.0, bend=2)
    trunk(s, cx, gy, top_y, 16, 11, lean=0.8)
    # knot
    s.ellipse(cx - 3, gy - 12, 1.6, 2.2, fill=K)
    canopy(s, rng, cx, ccy, A, B, bumps=15, bump_r=9.5, marks=14)
    for x, y in ((cx - 31, ccy + 8), (cx - 10, ccy - 12), (cx + 14, ccy + 3), (cx + 31, ccy + 20), (cx - 8, ccy + 22)):
        apple(s, x, y)
    for x in (24, 98, 37, 87, 110)[:ground_apples]:
        apple(s, x, ground_y(x) - 4.0, stem=False)


def draw(stage):
    rng = random.Random(4200 + stage)
    s = Svg()
    sky(s)
    hills(s)
    flowers = ()
    if stage >= 8:
        flowers = (14, 108, 30, 93, 44, 78)[:min(6, 1 + (stage - 8) // 3)]
    if stage >= 20:  # keep clear of fallen apples
        flowers = (11, 48, 76)
    ground(s, random.Random(11), stage, flowers)
    cx = W / 2
    gy = ground_y(cx)
    if stage == 0:  # dry seed, planted
        seed(s, cx, gy + 10.5, 5.4, 3.6, -14)
    elif stage == 1:  # swollen, first root showing
        roots(s, cx + 3, gy + 12, 6.5, 20, rng, n=1)
        seed(s, cx, gy + 10, 7.0, 4.8, -12)
    elif stage == 2:  # sprout folded over: a hook breaking the soil
        roots(s, cx - 3, gy + 5, 13, 30, rng, n=3)
        s.path(f"M{f(cx - 3)},{f(gy + 5)} C{f(cx - 4.5)},{f(gy - 12)} {f(cx + 6)},{f(gy - 15)} {f(cx + 7)},{f(gy - 6)}", stroke=K, w=2.6)
        leaf(s, cx + 7, gy - 8, 80, 7.5, fat=0.7, vein=False)
    elif stage == 3:  # starting to stand
        roots(s, cx, gy + 4, 15, 30, rng, n=3)
        s.path(f"M{f(cx)},{f(gy + 4)} C{f(cx - 1)},{f(gy - 12)} {f(cx + 3)},{f(gy - 21)} {f(cx + 9)},{f(gy - 23)}", stroke=K, w=2.6)
        leaf(s, cx + 8.5, gy - 23, -62, 8, fat=0.7, vein=False)
        leaf(s, cx + 8.5, gy - 23, 8, 8, fat=0.7, vein=False)
    elif stage == 4:  # standing up, seed leaves open
        roots(s, cx, gy + 3, 16, 28, rng, n=4)
        s.path(f"M{f(cx)},{f(gy + 3)} C{f(cx + 2.5)},{f(gy - 10)} {f(cx - 2)},{f(gy - 20)} {f(cx)},{f(gy - 30)}", stroke=K, w=2.6)
        leaf(s, cx, gy - 29.5, -152, 11, fat=0.72, vein=False)
        leaf(s, cx, gy - 29.5, -28, 11, fat=0.72, vein=False)
        leaf(s, cx, gy - 30, -92, 5.5, fat=0.6, vein=False)
    elif 5 <= stage <= 15:
        sapling(s, stage - 4, rng)
    elif 16 <= stage <= 19:
        youth_tree(s, stage - 15, rng)
    else:
        mature_tree(s, stage - 20, rng)
    when, what = STAGES[stage]
    return s.doc(f"Stage {stage + 1}: {when} - {what}")


def main():
    os.makedirs(OUT, exist_ok=True)
    for i in range(len(STAGES)):
        with open(os.path.join(OUT, f"stage-{i + 1:02d}.svg"), "w", encoding="utf8", newline="\n") as fh:
            fh.write(draw(i))
    print(f"wrote {len(STAGES)} stages to {os.path.relpath(OUT, ROOT)}")


if __name__ == "__main__":
    main()
