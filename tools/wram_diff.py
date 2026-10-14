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
    """Accept a framedump frame number or a `dump <tag>` name.

    Route work names its scene states with tags (`press a 8; dump talked`), so
    the same diff has to work on `<tag>.wram.bin` as well as framedump's
    `frame_%06d_wram.bin`; guessing which scheme a name belongs to is what makes
    a dialogue-flag comparison fail silently.
    """
    if isinstance(frame, str) and not frame.lstrip("-").isdigit():
        p = os.path.join(dirpath, "%s.wram.bin" % frame)
    else:
        p = os.path.join(dirpath, "frame_%06d_wram.bin" % int(frame))
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
    d, fa, fb = argv[0], argv[1], argv[2]
    a = load(d, fa)
    b = load(d, fb)
    r = runs(a, b)
    nbytes = sum(hi - lo + 1 for lo, hi in r)
    print("%s -> %s: %d changed bytes in %d run(s)" % (fa, fb, nbytes, len(r)))
    limit = 200 if "--full" not in argv else 100000
    for lo, hi in r[:limit]:
        vals = " ".join("%02X>%02X" % (a[lo + k], b[lo + k]) for k in range(min(hi - lo + 1, 8)))
        print("  $7E:%04X-%04X  %s" % (lo, hi, vals))
    if len(r) > limit:
        print("  ... (%d more runs)" % (len(r) - limit))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
