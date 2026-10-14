#!/usr/bin/env python3
"""Hexdump a window of a memory dump, flagging bytes that equal values you know.

  python tools/wram_peek.py build/dumps-walkhunt/town.wram.bin 480 96 --flag 10,3,5,100
  python tools/wram_peek.py build/dumps-walkhunt/town.wram.bin 4e0 48

A coordinate found by rate hunting tells you a block exists; the numbers on the
player's own menu tell you which bytes inside it are which. Lagoon's party
screen at the start of the game reads LV 1, HP 10/10, MP 3/3, STR 10, DEF 5,
GOLD 100, so those small integers, sitting next to each other in the same block
as the walking coordinate, are the state layout announcing itself. The flags mark
candidates; the differential dumps are still what confirms one.
"""
import sys


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    path, start, length = argv[0], int(argv[1], 16), int(argv[2], 16)
    flags = set()
    for i, a in enumerate(argv[3:], start=3):
        if a == "--flag":
            flags = {int(v) for v in argv[i + 1].split(",")}
    with open(path, "rb") as fh:
        data = fh.read()
    end = min(len(data), start + length)
    print("%s  window %04X..%04X  flags %s"
          % (path.split("/")[-1], start, end, sorted(flags) if flags else "-"))
    for row in range(start, end, 16):
        cells = []
        for off in range(row, min(row + 16, end)):
            v = data[off]
            cells.append(("%02X*" % v) if v in flags and flags else "%02X " % v)
        text = "".join(chr(c) if 32 <= c < 127 else "." for c in data[row:min(row + 16, end)])
        print("%04X  %s %s" % (row, " ".join(cells), text))
    return 0


if __name__ == "__main__":
    main(sys.argv[1:])
