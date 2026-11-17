"""Sobergotchi Markup Language (SML): parse screen files and lay them out.

A screen is a tree of boxes. Containers stack their children:
  <col>   top to bottom        <row>  left to right
  <stack> children on top of each other (same box), useful with if=""
  <box>   same as <col>
Leaves: <text>, <wrap>, <image>, <battery>, <dots>, <rule>, <spacer>, <list>

Sizing: in a <col> a child's height is h="20" (pixels), h="*" or h="2*" (share of
the space left), or left out (natural size). In a <row> the same goes for w="".
Common attributes: margin="t r b l", pad="t r b l" (containers), gap="n",
fill="ink|paper|g12|g25|g50|g75", border="n", radius="n", if="key" / if="!key",
align="left|center|right", valign="top|middle|bottom", font="name".
Text content can include {key} or {key:format} placeholders filled at runtime.
See docs/screen-markup.md for the full reference.
"""
import xml.etree.ElementTree as ET

from .fonts import to_device
from dataclasses import dataclass, field

T_GROUP, T_TEXT, T_WRAP, T_IMAGE, T_BATTERY, T_DOTS, T_RULE, T_LIST = range(8)
TYPE_NAMES = {"col": T_GROUP, "row": T_GROUP, "box": T_GROUP, "stack": T_GROUP, "spacer": T_GROUP,
              "text": T_TEXT, "wrap": T_WRAP, "image": T_IMAGE, "battery": T_BATTERY,
              "dots": T_DOTS, "rule": T_RULE, "list": T_LIST}
FILLS = {"none": 0, "ink": 1, "paper": 2, "g12": 3, "g25": 4, "g50": 5, "g75": 6}
FILL_LEVEL = {3: 2, 4: 4, 5: 8, 6: 12}  # inked cells out of 16 in a 4x4 Bayer pattern
HALIGN = {"left": 0, "center": 1, "right": 2}
VALIGN = {"top": 0, "middle": 1, "bottom": 2}
F_INVERT, F_DOTTED, F_VERTICAL = 1 << 4, 1 << 5, 1 << 6


class SmlError(Exception):
    pass


@dataclass
class Node:
    tag: str
    attrs: dict
    text: str = ""
    children: list = field(default_factory=list)
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0
    line: int = 0

    @property
    def type(self):
        return TYPE_NAMES[self.tag]


def _quad(v):
    if v is None:
        return (0, 0, 0, 0)
    parts = [int(p) for p in str(v).split()]
    if len(parts) == 1:
        return (parts[0],) * 4
    if len(parts) == 2:
        return (parts[0], parts[1], parts[0], parts[1])
    if len(parts) == 4:
        return tuple(parts)
    raise SmlError(f"bad box value '{v}' (use 1, 2 or 4 numbers)")


def parse(text, name):
    try:
        root = ET.fromstring(text)
    except ET.ParseError as e:
        raise SmlError(f"{name}: {e}")
    if root.tag != "screen":
        raise SmlError(f"{name}: the top element must be <screen>")

    def conv(el):
        if el.tag not in TYPE_NAMES and el.tag != "screen":
            raise SmlError(f"{name}: unknown element <{el.tag}>")
        body = to_device(" ".join((el.text or "").split()))
        n = Node(el.tag, dict(el.attrib), body)
        n.children = [conv(c) for c in el]
        return n

    scr = conv(root)
    scr.tag = "col"
    scr.attrs.setdefault("id", name)
    return scr


class Layout:
    def __init__(self, fonts, art_index, art_flat):
        self.fonts = {f.name: (i, f) for i, f in enumerate(fonts)}
        self.art_index = art_index
        self.art_flat = art_flat

    def font(self, n):
        name = n.attrs.get("font", "small")
        if name not in self.fonts:
            raise SmlError(f"unknown font '{name}' on <{n.tag}>")
        return self.fonts[name]

    def natural(self, n, axis):
        """Natural outer size along axis ('h' or 'w'), or None if it should stretch."""
        mt, mr, mb, ml = _quad(n.attrs.get("margin"))
        extra = (mt + mb) if axis == "h" else (ml + mr)
        if n.tag == "text" and axis == "h":
            _, f = self.font(n)
            return f.ascent + f.descent + 2 + extra
        if n.tag == "dots":
            return 7 + extra
        if n.tag == "rule":
            return 1 + extra if (axis == "h") != (n.attrs.get("dir") == "v") else None
        if n.tag == "battery":
            return (10 if axis == "h" else 26) + extra
        if n.tag == "image" and "src" in n.attrs:
            first, count = self.art_index[n.attrs["src"]]
            a = self.art_flat[first]
            return (a.height if axis == "h" else a.width) + extra
        if n.tag in ("col", "row", "box") and n.children:
            # natural size of a container = sum (along) or max (across) of fixed children
            along = (n.tag == "row") == (axis == "w")
            pt, pr, pb, pl = _quad(n.attrs.get("pad"))
            pad = (pt + pb) if axis == "h" else (pl + pr)
            sizes = []
            for c in n.children:
                s = self.size_attr(c, axis)
                if not isinstance(s, int):
                    return None
                sizes.append(s)
            gap = int(n.attrs.get("gap", 0))
            inner = (sum(sizes) + gap * (len(sizes) - 1)) if along else max(sizes)
            return inner + pad + extra
        return None

    def size_attr(self, n, axis):
        v = n.attrs.get(axis)
        if v is None:
            nat = self.natural(n, axis)
            return nat if nat is not None else ("*", 1)
        v = v.strip()
        if v.endswith("*"):
            return ("*", float(v[:-1] or 1))
        return int(v)

    def place(self, n, x, y, w, h):
        mt, mr, mb, ml = _quad(n.attrs.get("margin"))
        n.x, n.y, n.w, n.h = x + ml, y + mt, w - ml - mr, h - mt - mb
        if n.tag == "list":
            rowh = int(n.attrs.get("rowh", 20))
            self.stack_children(n, "col", n.x, n.y, n.w, rowh)
        elif n.children:
            pt, pr, pb, pl = _quad(n.attrs.get("pad"))
            ix, iy, iw, ih = n.x + pl, n.y + pt, n.w - pl - pr, n.h - pt - pb
            kind = "row" if n.tag == "row" else ("stack" if n.tag == "stack" else "col")
            self.stack_children(n, kind, ix, iy, iw, ih)

    def stack_children(self, n, kind, ix, iy, iw, ih):
        if kind == "stack":
            for c in n.children:
                self.place(c, ix, iy, iw, ih)
            return
        axis = "h" if kind == "col" else "w"
        total = ih if kind == "col" else iw
        gap = int(n.attrs.get("gap", 0))
        sizes = [self.size_attr(c, axis) for c in n.children]
        fixed = sum(s for s in sizes if isinstance(s, int)) + gap * (len(sizes) - 1)
        weights = sum(s[1] for s in sizes if not isinstance(s, int))
        free = max(0, total - fixed)
        resolved, used = [], 0
        flex_idx = [i for i, s in enumerate(sizes) if not isinstance(s, int)]
        for i, s in enumerate(sizes):
            if isinstance(s, int):
                resolved.append(s)
            else:
                part = int(free * s[1] / weights) if i != flex_idx[-1] else free - used
                used += part
                resolved.append(part)
        if fixed > total:
            raise SmlError(f"<{n.tag}> at line content '{n.attrs.get('id', '')}': children need {fixed}px but only {total}px fit")
        pos = iy if kind == "col" else ix
        for c, s in zip(n.children, resolved):
            if kind == "col":
                self.place(c, ix, pos, iw, s)
            else:
                self.place(c, pos, iy, s, ih)
            pos += s + gap


def flatten(n, layout):
    """Pre-order list of node records for the firmware and the preview."""
    out = []

    def walk(n):
        rec = {"node": n, "type": n.type, "font": 0, "flags": 0, "fill": FILLS[n.attrs.get("fill", "none")],
               "border": int(n.attrs.get("border", 0)), "radius": int(n.attrs.get("radius", 0)),
               "x": n.x, "y": n.y, "w": n.w, "h": n.h, "a": 0, "b": 0, "text": None,
               "cond": n.attrs.get("if"), "skip": 0}
        default_align = "center" if n.tag in ("image", "dots") else "left"
        rec["flags"] |= HALIGN[n.attrs.get("align", default_align)]
        rec["flags"] |= VALIGN[n.attrs.get("valign", "middle" if n.tag != "wrap" else "top")] << 2
        if rec["fill"] == 1:
            rec["flags"] |= F_INVERT
        if n.tag == "image" and n.attrs.get("draw") == "paper":  # knock a shape out of what is behind
            rec["flags"] |= F_INVERT
        if n.tag in ("text", "wrap"):
            rec["font"] = layout.font(n)[0]
            rec["text"] = n.text
            if n.tag == "wrap":
                rec["a"] = int(n.attrs.get("lh", layout.font(n)[1].ascent + layout.font(n)[1].descent + 3))
                rec["b"] = 0
                rec["text2"] = n.attrs.get("page", "0")
        elif n.tag == "image":
            src = n.attrs.get("src")
            if src not in layout.art_index:
                raise SmlError(f"<image src='{src}'>: no such art")
            rec["a"], rec["b"] = layout.art_index[src]
            rec["text"] = n.attrs.get("index")
        elif n.tag == "battery":
            rec["text"] = n.attrs.get("pct", "{battery.pct}")
            rec["text2"] = n.attrs.get("charging", "battery.charging")
        elif n.tag == "dots":
            rec["a"] = int(n.attrs.get("count", 4))
            rec["text"] = n.attrs.get("index", "{screen.index}")
        elif n.tag == "rule":
            if n.attrs.get("dir") == "v":
                rec["flags"] |= F_VERTICAL
            if n.attrs.get("style") == "dotted":
                rec["flags"] |= F_DOTTED
        elif n.tag == "list":
            rec["a"] = int(n.attrs.get("rowh", 20))
            rec["text"] = n.attrs.get("bind")
            rec["text2"] = n.attrs.get("sel", "-1")
            if not rec["text"]:
                raise SmlError("<list> needs bind=\"name\"")
        out.append(rec)
        start = len(out)
        for c in n.children:
            walk(c)
        rec["skip"] = len(out) - start

    walk(n)
    return out


def compile_screen(text, name, layout, width=122, height=250):
    root = parse(text, name)
    layout.place(root, 0, 0, width, height)
    return root.attrs.get("id", name), flatten(root, layout)


def c_str(s):
    """C string literal; bytes >= 127 become octal escapes (they are the extra glyphs)."""
    if s is None:
        return "nullptr"
    out = []
    for ch in s:
        if ch in "\\\"":
            out.append("\\" + ch)
        elif ord(ch) >= 127:
            out.append("\\%03o" % ord(ch))
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


def emit_header(screens):
    out = ["// Generated by tools/build_ui.py from screens/*.sml. Do not edit.", "#pragma once",
           '#include "../ui/ui_types.h"', ""]
    for sid, recs in screens:
        cid = sid.replace("-", "_")
        out.append(f"static const UiNode SCREEN_{cid}_nodes[] = {{")
        for r in recs:
            out.append("  {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %s, %s, %s, %d}, // <%s>" % (
                r["type"], r["font"], r["flags"], r["fill"], r["border"], r["radius"],
                r["x"], r["y"], r["w"], r["h"], r["a"], r["b"],
                c_str(r["text"]), c_str(r.get("text2")), c_str(r["cond"]), r["skip"], r["node"].tag))
        out.append("};")
    out.append("")
    out.append("static const UiScreen UI_SCREENS[] = {")
    for sid, recs in screens:
        cid = sid.replace("-", "_")
        out.append(f"  {{\"{sid}\", SCREEN_{cid}_nodes, {len(recs)}}},")
    out.append("};")
    for i, (sid, _) in enumerate(screens):
        out.append(f"#define SCREEN_{sid.upper().replace('-', '_')} {i}")
    out.append(f"#define SCREEN_COUNT {len(screens)}")
    return "\n".join(out) + "\n"
