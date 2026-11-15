#!/usr/bin/env python3
"""Generate the 26 tree-stage drawings as SVG files in art/tree/.

The SVGs are the source art: you can open and redraw any of them in Inkscape,
Figma or Illustrator. Keep the two layers: <g id="ink"> for black line work and
<g id="shade"> for grey areas (greys become dither textures on the e-paper).
Re-running this script overwrites art/tree/*.svg.
"""
import math
import os
import random

W, H = 122, 114
GROUND = 95  # ground line height at the centre
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "art", "tree")

# Grey levels (become dither textures): lower = darker
G_SOIL, G_LEAF, G_LEAF_DARK, G_CANOPY, G_CANOPY_DARK, G_TRUNK, G_SEED = 236, 200, 130, 226, 186, 118, 120

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


def grey(v):
    return f"rgb({v},{v},{v})"


def ground_y(x):
    """Gentle mound: highest in the middle."""
    t = (x - W / 2) / (W / 2)
    return GROUND + 4 * t * t


class Svg:
    def __init__(self):
        self.ink, self.shade = [], []

    def doc(self, title):
        return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">\n'
                f'<title>{title}</title>\n'
                f'<g id="shade">\n' + "\n".join(self.shade) + '\n</g>\n'
                f'<g id="ink" fill="none" stroke="#000" stroke-linecap="round" stroke-linejoin="round">\n'
                + "\n".join(self.ink) + '\n</g>\n</svg>\n')


def f(v):
    return f"{v:.2f}".rstrip("0").rstrip(".")


def path(pts, close=False):
    d = "M" + " L".join(f"{f(x)},{f(y)}" for x, y in pts)
    return d + (" Z" if close else "")


def scene(s, rng, stage):
    # sun and a cloud, drawn first so a big canopy covers them
    sx, sy, sr = 101, 15, 6.5
    s.shade.append(f'<circle cx="{sx}" cy="{sy}" r="{sr}" fill="{grey(214)}"/>')
    s.ink.append(f'<circle cx="{sx}" cy="{sy}" r="{sr}" stroke-width="1.3"/>')
    for k in range(8):
        a = k * math.pi / 4 + 0.2
        s.ink.append(f'<path d="M{f(sx + math.cos(a) * (sr + 2.5))},{f(sy + math.sin(a) * (sr + 2.5))} L{f(sx + math.cos(a) * (sr + 5.5))},{f(sy + math.sin(a) * (sr + 5.5))}" stroke-width="1.2"/>')
    cloud = [(17, 22, 5), (24, 18, 6.5), (31, 22, 5), (22, 24, 4), (28, 24.5, 4)]
    for x, y, r in cloud:
        s.ink.append(f'<circle cx="{x}" cy="{y}" r="{r}" stroke-width="1.2"/>')
    for x, y, r in cloud:
        s.ink.append(f'<circle cx="{x}" cy="{y}" r="{r - 0.7}" fill="#fff" stroke="none"/>')
    s.ink.append('<path d="M12.5,26.5 L35.5,26.5" stroke="#fff" stroke-width="2"/>')
    s.ink.append('<path d="M13,27.2 L35,27.2" stroke-width="1.2"/>')
    # soil
    soil = [(x, ground_y(x)) for x in range(0, W + 1, 2)] + [(W, H), (0, H)]
    s.shade.append(f'<path d="{path(soil, True)}" fill="{grey(G_SOIL)}"/>')
    # ground line
    gl = [(x, ground_y(x)) for x in range(0, W + 1, 2)]
    s.ink.append(f'<path d="{path(gl)}" stroke-width="1.6"/>')
    # pebbles in the soil
    for _ in range(9):
        x = rng.uniform(4, W - 4)
        y = rng.uniform(ground_y(x) + 6, H - 3)
        rx = rng.uniform(1.4, 2.6)
        s.ink.append(f'<ellipse cx="{f(x)}" cy="{f(y)}" rx="{f(rx)}" ry="{f(rx * 0.65)}" stroke-width="1.05"/>')
    # grass tufts on the ground
    tufts = [8, 19, 33, 88, 101, 114] if stage >= 2 else [10, 24, 96, 112]
    for x in tufts:
        x += rng.uniform(-2, 2)
        gy = ground_y(x)
        for dx, h, lean in ((-2, 4, -1.6), (0, 6, 0.3), (2, 4.5, 1.8)):
            s.ink.append(f'<path d="M{f(x + dx)},{f(gy)} Q{f(x + dx + lean * 0.3)},{f(gy - h * 0.6)} {f(x + dx + lean)},{f(gy - h)}" stroke-width="1.1"/>')


def seed(s, cx, cy, rx, ry, angle, swollen=False):
    s.shade.append(f'<ellipse cx="{f(cx)}" cy="{f(cy)}" rx="{f(rx)}" ry="{f(ry)}" transform="rotate({angle} {f(cx)} {f(cy)})" fill="{grey(G_SEED)}"/>')
    s.ink.append(f'<ellipse cx="{f(cx)}" cy="{f(cy)}" rx="{f(rx)}" ry="{f(ry)}" transform="rotate({angle} {f(cx)} {f(cy)})" stroke-width="1.5"/>')
    # seam
    s.ink.append(f'<path d="M{f(cx - rx * 0.7)},{f(cy)} Q{f(cx)},{f(cy - ry * 0.55)} {f(cx + rx * 0.7)},{f(cy)}" transform="rotate({angle} {f(cx)} {f(cy)})" stroke-width="1"/>')
    # highlight (white) on the ink layer = erase
    s.ink.append(f'<ellipse cx="{f(cx - rx * 0.35)}" cy="{f(cy - ry * 0.45)}" rx="{f(rx * 0.25)}" ry="{f(ry * 0.18)}" transform="rotate({angle} {f(cx)} {f(cy)})" fill="#fff" stroke="none"/>')


def soil_pocket(s, cx, cy, r):
    # a darker pocket of dug soil around a planted seed
    s.shade.append(f'<ellipse cx="{f(cx)}" cy="{f(cy)}" rx="{f(r * 1.6)}" ry="{f(r * 1.05)}" fill="#fff"/>')
    s.ink.append(f'<ellipse cx="{f(cx)}" cy="{f(cy)}" rx="{f(r * 1.6)}" ry="{f(r * 1.05)}" stroke-width="0.9" stroke-dasharray="1.2 1.6"/>')


def roots(s, x, y, depth, spread, rng, n=3):
    for i in range(n):
        ang = math.radians(90 + (i - (n - 1) / 2) * spread)
        L = depth * rng.uniform(0.75, 1.0)
        ex, ey = x + math.cos(ang) * L * 0.6, y + math.sin(ang) * L
        mx, my = x + math.cos(ang) * L * 0.2 + rng.uniform(-2, 2), y + L * 0.45
        s.ink.append(f'<path d="M{f(x)},{f(y)} Q{f(mx)},{f(my)} {f(ex)},{f(min(ey, H - 2))}" stroke-width="1"/>')


def leaf(s, bx, by, angle_deg, length, width, dark=False, rib=True):
    a = math.radians(angle_deg)
    ux, uy = math.cos(a), math.sin(a)
    nx, ny = -uy, ux
    tx, ty = bx + ux * length, by + uy * length
    c1 = (bx + ux * length * 0.45 + nx * width, by + uy * length * 0.45 + ny * width)
    c2 = (bx + ux * length * 0.45 - nx * width, by + uy * length * 0.45 - ny * width)
    d = f"M{f(bx)},{f(by)} Q{f(c1[0])},{f(c1[1])} {f(tx)},{f(ty)} Q{f(c2[0])},{f(c2[1])} {f(bx)},{f(by)} Z"
    s.shade.append(f'<path d="{d}" fill="{grey(G_LEAF_DARK if dark else G_LEAF)}"/>')
    s.ink.append(f'<path d="{d}" stroke-width="1.35" fill="#fff" fill-opacity="0"/>')
    if rib and length >= 8:
        s.ink.append(f'<path d="M{f(bx + ux * 1.5)},{f(by + uy * 1.5)} L{f(bx + ux * length * 0.72)},{f(by + uy * length * 0.72)}" stroke-width="0.9"/>')


def cotyledons(s, x, y, size, open_amt):
    # two round seed leaves; open_amt 0 = closed upward, 1 = flat open
    for side in (-1, 1):
        ang = -90 + side * (12 + 70 * open_amt)
        leaf(s, x, y, ang, size, size * 0.62, rib=False)


def stem(s, pts, width):
    d = "M" + " ".join(f"{f(x)},{f(y)}" for x, y in pts[:1]) + " " + " ".join(
        f"Q{f(pts[i][0])},{f(pts[i][1])} {f((pts[i][0] + pts[i + 1][0]) / 2)},{f((pts[i][1] + pts[i + 1][1]) / 2)}" for i in range(1, len(pts) - 1)
    ) + f" L{f(pts[-1][0])},{f(pts[-1][1])}"
    s.ink.append(f'<path d="{d}" stroke-width="{f(width)}"/>')


def apple(s, x, y, r=4.4, leafy=True):
    s.ink.append(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r)}" fill="#000" stroke="#fff" stroke-width="1.2" paint-order="stroke"/>')
    s.ink.append(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r)}" fill="#000" stroke="none"/>')
    s.ink.append(f'<circle cx="{f(x - r * 0.38)}" cy="{f(y - r * 0.38)}" r="{f(r * 0.3)}" fill="#fff" stroke="none"/>')
    s.ink.append(f'<path d="M{f(x + 0.3)},{f(y - r + 0.8)} q0.2,-2 1.4,-2.9" stroke-width="1.1"/>')
    if leafy:
        s.ink.append(f'<path d="M{f(x + 1.2)},{f(y - r - 1.4)} q2.4,-1.8 4.2,-0.4 q-2.2,1.6 -4.2,0.4 Z" fill="#fff" stroke-width="1"/>')


def trunk(s, base_x, base_y, top_y, w_base, w_top, lean=0):
    lx0, rx0 = base_x - w_base / 2, base_x + w_base / 2
    lx1, rx1 = base_x + lean - w_top / 2, base_x + lean + w_top / 2
    # root flare
    d = (f"M{f(lx0 - 3)},{f(base_y + 0.5)} Q{f(lx0)},{f(base_y - 1)} {f(lx0 + 0.6)},{f(base_y - 6)} "
         f"L{f(lx1)},{f(top_y)} L{f(rx1)},{f(top_y)} L{f(rx0 - 0.6)},{f(base_y - 6)} "
         f"Q{f(rx0)},{f(base_y - 1)} {f(rx0 + 3)},{f(base_y + 0.5)} Z")
    s.shade.append(f'<path d="{d}" fill="{grey(G_TRUNK)}"/>')
    s.ink.append(f'<path d="{d}" stroke-width="1.5" fill="#fff" fill-opacity="0"/>')
    # bark lines
    for k in range(3):
        y0 = base_y - 8 - k * (base_y - top_y) / 4
        x0 = base_x + lean * (base_y - y0) / max(1, base_y - top_y) + (k - 1) * w_base * 0.18
        s.ink.append(f'<path d="M{f(x0)},{f(y0)} l{f(0.3)},{f(-4)}" stroke-width="0.9"/>')


def branch(s, x0, y0, x1, y1, w, bend=0.0):
    mx, my = (x0 + x1) / 2 + bend, (y0 + y1) / 2 - abs(bend) * 0.5
    s.ink.append(f'<path d="M{f(x0)},{f(y0)} Q{f(mx)},{f(my)} {f(x1)},{f(y1)}" stroke-width="{f(w)}"/>')


def canopy(s, lobes):
    """lobes: list of (cx, cy, r). Union outline with soft shading."""
    # shading: base, shadow under the lower half, white highlight on the upper left
    clip_id = f"c{len(s.shade)}"
    circles = "".join(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r)}"/>' for x, y, r in lobes)
    s.shade.append(f'<clipPath id="{clip_id}">{circles}</clipPath>')
    ys = [y for _, y, _ in lobes]
    cy = sum(ys) / len(ys)
    top = min(y - r for _, y, r in lobes)
    bot = max(y + r for _, y, r in lobes)
    s.shade.append(f'<g clip-path="url(#{clip_id})">'
                   f'<rect x="0" y="0" width="{W}" height="{H}" fill="{grey(G_CANOPY)}"/>'
                   f'<rect x="0" y="{f(cy + (bot - cy) * 0.35)}" width="{W}" height="{H}" fill="{grey(G_CANOPY_DARK)}"/>'
                   + "".join(f'<circle cx="{f(x - r * 0.3)}" cy="{f(y - r * 0.35)}" r="{f(r * 0.45)}" fill="#fff"/>' for x, y, r in lobes if y < cy)
                   + '</g>')
    # outline of the union: thick strokes under, white fills on top
    for x, y, r in lobes:
        s.ink.append(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r)}" stroke-width="1.6"/>')
    for x, y, r in lobes:
        s.ink.append(f'<circle cx="{f(x)}" cy="{f(y)}" r="{f(r - 0.85)}" fill="#fff" stroke="none"/>')
    # a few leafy scallop marks inside for texture
    return top, bot


def leaf_marks(s, lobes, rng, n):
    for _ in range(n):
        x, y, r = rng.choice(lobes)
        a = rng.uniform(0, math.tau)
        d = rng.uniform(0.25, 0.7) * r
        px, py = x + math.cos(a) * d, y + math.sin(a) * d
        s.ink.append(f'<path d="M{f(px - 2.4)},{f(py)} q1.2,1.6 2.4,0 q1.2,1.6 2.4,0" stroke-width="1"/>')


def sapling(s, month, rng):
    """month 1..11: stem with branches and leaves."""
    cx = W / 2
    gy = ground_y(cx)
    height = 36 + month * 5.0
    top = gy - height
    sway = [(cx, gy), (cx + 1.2, gy - height * 0.35), (cx - 1.5, gy - height * 0.7), (cx + 0.5, top)]
    stem_w = 2.2 + month * 0.14
    stem(s, sway, stem_w)

    def stem_x(y):
        t = (gy - y) / height
        return cx + 1.2 * math.sin(t * math.pi) - 1.0 * t

    branches_at = [m for m in (1, 3, 6, 9) if m <= month]
    nb = len(branches_at)
    tips = []
    for k, m in enumerate(branches_at):
        side = -1 if k % 2 == 0 else 1
        by = gy - height * (0.38 + 0.13 * k)
        bx = stem_x(by)
        age = month - m
        L = 12 + age * 1.8
        ex, ey = bx + side * L, by - L * 0.55
        branch(s, bx, by, ex, ey, 1.4 + age * 0.06, bend=side * 1.5)
        tips.append((ex, ey, side, bx, by))
    # leaf positions: branch tips first, then along branches and stem, then the stem top
    spots = []
    for ex, ey, side, bx, by in tips:
        spots.append((ex, ey, -90 + side * 50))
    spots.append((stem_x(top) + 0.5, top + 1, -95))
    for ex, ey, side, bx, by in tips:
        mx, my = (ex + bx) / 2, (ey + by) / 2
        spots.append((mx, my, -90 + side * 95))
    for k in range(6):
        y = gy - height * (0.3 + 0.1 * k)
        side = 1 if k % 2 == 0 else -1
        spots.append((stem_x(y), y, -90 + side * 65))
    leaf_size = 12 + min(month, 8) * 0.4
    for i in range(month):
        x, y, ang = spots[i]
        leaf(s, x, y, ang + rng.uniform(-6, 6), leaf_size, leaf_size * 0.45, dark=(i % 3 == 2))
    if month <= 2:
        cotyledons(s, stem_x(gy - 12), gy - 12, 6, 1.0)


def youth_tree(s, year, rng, apples):
    cx = W / 2
    gy = ground_y(cx)
    g = year  # 1..4
    tw = 6 + g * 0.9
    canopy_bottom = gy - 30 - g * 2
    trunk(s, cx, gy, canopy_bottom + 6, tw, tw * 0.65, lean=0.5)
    # main forks into the canopy
    branch(s, cx, canopy_bottom + 10, cx - 9 - g, canopy_bottom - 6, 1.8)
    branch(s, cx + 0.5, canopy_bottom + 8, cx + 10 + g, canopy_bottom - 8, 1.8)
    R = 19 + g * 2.6
    ccy = canopy_bottom - R * 0.8
    lobes = [(cx, ccy - R * 0.25, R * 0.72), (cx - R * 0.62, ccy + R * 0.1, R * 0.58), (cx + R * 0.62, ccy + R * 0.05, R * 0.6),
             (cx - R * 0.25, ccy + R * 0.42, R * 0.55), (cx + R * 0.3, ccy + R * 0.45, R * 0.52)]
    canopy(s, lobes)
    leaf_marks(s, [l for l in lobes if l[1] < ccy], rng, g)
    spots = [(cx + R * 0.5, ccy + R * 0.48), (cx - R * 0.42, ccy - R * 0.5), (cx - R * 0.7, ccy + R * 0.3),
             (cx - R * 0.2, ccy + R * 0.7), (cx + R * 0.8, ccy - R * 0.25)]
    for i in range(apples):
        apple(s, *spots[i])


def mature_tree(s, ground_apples, rng):
    cx = W / 2
    gy = ground_y(cx)
    canopy_bottom = gy - 34
    trunk(s, cx, gy, canopy_bottom + 4, 11, 7, lean=0.6)
    for dx, dy, w in ((-20, -14, 2.6), (19, -16, 2.6), (-4, -22, 2.2), (9, -20, 2.0)):
        branch(s, cx + 0.6, canopy_bottom + 8, cx + dx, canopy_bottom + dy, w, bend=dx * 0.08)
    lobes = [(61, 26, 21), (40, 38, 17), (83, 37, 18), (27, 54, 13), (96, 53, 13), (48, 57, 15), (74, 58, 15), (61, 45, 18),
             (61, 14, 12), (44, 21, 12), (79, 22, 12)]
    canopy(s, lobes)
    leaf_marks(s, lobes, rng, 12)
    for x, y in ((39, 46), (74, 31), (88, 54), (53, 20), (64, 62)):
        apple(s, x, y)
    for i, x in enumerate((22, 99, 33, 88, 109)[:ground_apples]):
        apple(s, x, ground_y(x) - 3.6, r=3.8, leafy=False)


def draw(stage):
    rng = random.Random(1000 + stage)
    s = Svg()
    scene(s, random.Random(7), stage)
    cx = W / 2
    gy = ground_y(cx)
    if stage == 0:  # dry seed, planted
        soil_pocket(s, cx, gy + 10, 7)
        seed(s, cx, gy + 10, 7.0, 4.4, -12)
    elif stage == 1:  # swollen, first root
        soil_pocket(s, cx, gy + 10, 9)
        roots(s, cx + 2, gy + 12, 6, 25, rng, n=1)
        seed(s, cx, gy + 9, 9.0, 6.4, -10, swollen=True)
        s.ink.append(f'<path d="M{f(cx - 1)},{f(gy + 5)} l2,3 l-1,2 l2,2" stroke-width="1"/>')
    elif stage == 2:  # sprout folded over: a hook breaking the soil
        roots(s, cx - 3, gy + 6, 13, 30, rng, n=3)
        stem(s, [(cx - 3, gy + 6), (cx - 4, gy - 8), (cx + 4, gy - 16), (cx + 8, gy - 7)], 2.2)
        seed(s, cx + 8.5, gy - 4, 3.6, 2.6, 75)
    elif stage == 3:  # starting to stand
        roots(s, cx, gy + 4, 15, 32, rng, n=3)
        stem(s, [(cx, gy + 4), (cx + 1, gy - 12), (cx + 7, gy - 22), (cx + 9, gy - 23)], 2.2)
        leaf(s, cx + 8.5, gy - 23, -55, 6, 3.2, rib=False)
        leaf(s, cx + 8.5, gy - 23, -10, 6, 3.2, rib=False)
    elif stage == 4:  # standing up, seed leaves open
        roots(s, cx, gy + 3, 16, 30, rng, n=4)
        stem(s, [(cx, gy + 3), (cx + 1.2, gy - 14), (cx - 0.8, gy - 26), (cx, gy - 30)], 2.2)
        cotyledons(s, cx, gy - 30, 8.5, 1.0)
    elif 5 <= stage <= 15:
        sapling(s, stage - 4, rng)
    elif 16 <= stage <= 19:
        youth_tree(s, stage - 15, rng, apples=stage - 15)
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
