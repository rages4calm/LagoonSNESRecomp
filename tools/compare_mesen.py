#!/usr/bin/env python3
"""Side-by-side: this port versus the Mesen core at matched frames.

Each row is one frame: the port's scene dump on the left, Mesen's screenshot of
the same frame on the right, plus how much of WRAM disagrees. Frame numbers come
from `tools/add_dumps.py`'s printed schedule, because a dump lands on the command
boundary nearest the frame it was asked for, not on that frame.

  python tools/compare_mesen.py 3187=c-3000 6134=c-6000 ...
"""
import os
import sys

from PIL import Image, ImageDraw


def port_rgb(tag, dump_dir):
    path = os.path.join(dump_dir, tag + ".fb.bgrx")
    w, h = 256, 224
    if not os.path.exists(path):
        alt = os.path.join(dump_dir, tag + ".fb.bmp")
        if os.path.exists(alt):
            return Image.open(alt).convert("RGB").crop((0, 0, w, h)), alt
        return None, path
    with open(path, "rb") as f:
        raw = f.read()
    img = Image.frombytes("RGBX", (w, h), raw, "raw", "BGRX")
    return img.convert("RGB"), path


def main(argv):
    dump_dir = os.environ.get("DUMPS", "build/dumps-cmp-aot")
    # mesen_reference.ps1 names its output build/mesen-<Tag>; the tag is chosen
    # per campaign, so the prefix is not the tool's call to make.
    prefix = os.environ.get("MESEN_PREFIX", "mesen-cmp")
    rows = []
    for spec in argv:
        frame, tag = spec.split("=")
        rows.append((int(frame), tag))
    gap, label = 12, 22
    sheet = Image.new("RGB", (256 * 2 + gap * 3, (224 + label + 16) * len(rows) + gap), (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    for i, (frame, tag) in enumerate(rows):
        left, lpath = port_rgb(tag, dump_dir)
        rpath = "build/%s%d/last.png" % (prefix, frame)
        right = Image.open(rpath).convert("RGB").crop((0, 0, 256, 224)) if os.path.exists(rpath) else None
        y = gap + i * (224 + label + 16)
        draw.text((gap, y), "frame %d   port (left) vs Mesen (right)" % frame, fill=(235, 235, 235))
        y += label
        if left:
            sheet.paste(left, (gap, y))
        if right:
            sheet.paste(right, (gap * 2 + 256, y))
        wp = "build/%s%d/wram.bin" % (prefix, frame)
        pp = os.path.join(dump_dir, tag + ".wram.bin")
        note = "no comparison"
        if os.path.exists(wp) and os.path.exists(pp):
            a, b = open(wp, "rb").read(), open(pp, "rb").read()
            n = sum(1 for x, c in zip(a, b) if x != c)
            note = "wram differs in %d of %d bytes" % (n, len(a))
        elif not os.path.exists(lpath):
            note = "missing port dump: " + lpath
        draw.text((gap, y + 228), note, fill=(255, 200, 120))
    out = os.path.join(dump_dir, "compare_mesen.png")
    sheet.save(out)
    print(out)


if __name__ == "__main__":
    main(sys.argv[1:])
