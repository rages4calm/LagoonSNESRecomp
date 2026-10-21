#!/usr/bin/env python3
"""List the entity records in a WRAM scene dump.

Usage:
  python tools/entities.py <dump.wram.bin> [--near X,Y [--radius N]] [--all]

Records are 0x40 bytes apart from $7E:0500 (documented in docs/DEVELOPMENT.md: the
player is record 0 and everything after it repeats the same header shape). Only
records whose state byte is non-zero are listed unless --all, because an empty
slot keeps whatever coordinates it had when it was last alive, which reads as a
phantom entity otherwise.

`screen` is the position the engine derived for drawing ($050C/$050E, signed 16);
an entity with |screen| well outside 0..255 / 0..223 is off view, which is what
decides whether an attack can ever reach it.
"""
import argparse
import os
import struct
import sys

BASE = 0x0500
STRIDE = 0x40
COUNT = 40  # records the dumps reliably cover ($0500..$0FA0)


def main(argv):
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("dump")
    ap.add_argument("--near", default="", help="world X,Y to sort and filter by")
    ap.add_argument("--radius", type=int, default=200)
    ap.add_argument("--all", action="store_true", help="include empty slots")
    args = ap.parse_args(argv)

    if not os.path.isfile(args.dump):
        print(f"not found: {args.dump}", file=sys.stderr)
        return 2
    ram = open(args.dump, "rb").read()

    px = py = None
    rows = []
    for i in range(COUNT):
        off = BASE + i * STRIDE
        if off + STRIDE > len(ram):
            break
        rec = ram[off:off + STRIDE]
        state = rec[0] | (rec[1] << 8)
        x, y = struct.unpack_from("<HH", rec, 0x02)
        sx, sy = struct.unpack_from("<hh", rec, 0x0C)
        vx, vy = struct.unpack_from("<hh", rec, 0x06)
        sprite = rec[0x1D]
        if i == 0:
            px, py = x, y
        if not state and not args.all:
            continue
        rows.append((i, off, state, x, y, sx, sy, vx, vy, sprite))

    if args.near:
        try:
            nx, ny = (int(v) for v in args.near.split(","))
        except ValueError:
            print("--near wants X,Y", file=sys.stderr)
            return 2
        rows = [r for r in rows if abs(r[3] - nx) <= args.radius and abs(r[4] - ny) <= args.radius]
        rows.sort(key=lambda r: (r[3] - nx) ** 2 + (r[4] - ny) ** 2)

    print(f"{args.dump}: player at ({px},{py}); {len(rows)} live record(s), stride 0x40")
    print("  rec  addr   state     X     Y   screenX screenY   vx  vy  spr")
    for i, off, state, x, y, sx, sy, vx, vy, sprite in rows:
        tag = " <- Nasir" if i == 0 else ""
        print(f"  {i:3d}  ${off:04X}  {state:04X}  {x:5d}  {y:5d}  {sx:7d} {sy:7d} {vx:4d} {vy:4d}  {sprite:02X}{tag}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
