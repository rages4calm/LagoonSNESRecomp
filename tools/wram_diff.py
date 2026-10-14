#!/usr/bin/env python3
"""Diff WRAM snapshots from a host framedump and show which bytes change.

Usage:
  python tools/wram_diff.py <dir> <frameA> <frameB> [--full]

Lists changed 16-byte runs (or a count) so a spin loop is recognizable: a real
game frame touches many regions; a stuck loop touches a handful of bytes.
"""
import os
import sys


def load(dirpath, frame):
    p = os.path.join(dirpath, "frame_%06d_wram.bin" % frame)
    with open(p, "rb") as f:
        return f.read()


def runs(a, b, wram_mask=0x2000):
    """Changed addresses, grouped into runs, reported as SNES $7E:xxxx."""
    out = []
    start = None
    prev = None
    for i in range(min(len(a), len(b))):
        if a[i] != b[i]:
            if start is None:
                start = i
            prev = i
            continue
        if start is not None:
            out.append((start, prev))
            start = None
    if start is not None:
        out.append((start, prev))
    return out


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    d, fa, fb = argv[0], int(argv[1]), int(argv[2])
    a = load(d, fa)
    b = load(d, fb)
    r = runs(a, b)
    nbytes = sum(hi - lo + 1 for lo, hi in r)
    print("frame %d -> %d: %d changed bytes in %d run(s)" % (fa, fb, nbytes, len(r)))
    limit = 200 if "--full" not in argv else 100000
    for lo, hi in r[:limit]:
        vals = " ".join("%02X>%02X" % (a[lo + k], b[lo + k]) for k in range(min(hi - lo + 1, 8)))
        print("  $7E:%04X-%04X  %s" % (lo, hi, vals))
    if len(r) > limit:
        print("  ... (%d more runs)" % (len(r) - limit))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
