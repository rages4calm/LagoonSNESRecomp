#!/usr/bin/env python3
"""Stack named scene dumps from one run into a single labelled sheet.

  python tools/tag_sheet.py build/dumps-qreg-aot town moved menu back

Rows are the tags in the order asked for; each carries the frame it was taken
at (from the dump's own .info.json) so a screenshot cannot be quoted without its
moment. `--cols N` lays the rows out side by side instead.
"""
import json
import os
import sys

from PIL import Image, ImageDraw

W, H = 256, 224
GAP, LABEL = 10, 20


def frame_of(dump_dir, tag):
    path = os.path.join(dump_dir, tag + ".info.json")
    if os.path.exists(path):
        try:
            with open(path) as f:
                return str(json.load(f).get("frame", "?"))
        except (ValueError, OSError):
            pass
    return "?"


def load(dump_dir, tag):
    bgrx = os.path.join(dump_dir, tag + ".fb.bgrx")
    if os.path.exists(bgrx):
        with open(bgrx, "rb") as f:
            return Image.frombytes("RGBX", (W, H), f.read(), "raw", "BGRX").convert("RGB")
    bmp = os.path.join(dump_dir, tag + ".fb.bmp")
    if os.path.exists(bmp):
        return Image.open(bmp).convert("RGB").crop((0, 0, W, H))
    return None


def main(argv):
    cols = 1
    if argv and argv[0] == "--cols":
        cols, argv = int(argv[1]), argv[2:]
    if len(argv) < 2:
        sys.exit(__doc__)
    dump_dir, tags = argv[0], argv[1:]
    rows = (len(tags) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (W + GAP) + GAP, rows * (H + LABEL + GAP) + GAP), (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    missing = []
    for i, tag in enumerate(tags):
        img = load(dump_dir, tag)
        x = GAP + (i % cols) * (W + GAP)
        y = GAP + (i // cols) * (H + LABEL + GAP)
        draw.text((x, y), "%s  frame %s" % (tag, frame_of(dump_dir, tag)), fill=(235, 235, 235))
        if img is None:
            missing.append(tag)
            continue
        sheet.paste(img, (x, y + LABEL))
    out = os.path.join(dump_dir, "sheet.png")
    sheet.save(out)
    print(out)
    if missing:
        print("missing: " + " ".join(missing))


if __name__ == "__main__":
    main(sys.argv[1:])
