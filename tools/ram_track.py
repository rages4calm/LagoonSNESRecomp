#!/usr/bin/env python3
"""Print chosen WRAM addresses for every dump matching a pattern, in frame order.

  python tools/ram_track.py build/dumps-walke 'e0*' 502,504
  python tools/ram_track.py build/dumps-qreg-aot 'town' 502,504 --fixed

dump_series.py finds candidates by their behaviour; this reads them directly,
which is the check that turns a candidate into a known variable: does the value
move when it should, hold when it should, and agree with what the screen shows.
Values print raw by default; `--fixed` adds an 8.8 fixed-point reading for
variables that really are sub-pixel. Lagoon's entity positions are not: $0502 and
$0504 are 16-bit pixel coordinates and the engine adds a 16-bit velocity to them,
so an earlier "8.8" guess here was an off-by-one read of the same variable
($0501 instead of $0502) and is recorded here so it does not get re-derived.
"""
import glob
import json
import os
import sys


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    root, pattern = argv[0], argv[1]
    addrs = [int(a, 16) for a in argv[2].split(",")]
    fixed = "--fixed" in argv
    rows = []
    for path in sorted(glob.glob(os.path.join(root, pattern + ".wram.bin"))):
        tag = os.path.basename(path)[:-len(".wram.bin")]
        info = os.path.join(root, tag + ".info.json")
        frame = None
        if os.path.exists(info):
            try:
                with open(info) as fh:
                    frame = json.load(fh).get("frame")
            except ValueError:
                pass
        with open(path, "rb") as fh:
            data = fh.read()
        rows.append((frame if frame is not None else -1, tag, data))
    if not rows:
        sys.exit("no dumps matched %s" % os.path.join(root, pattern))
    rows.sort()
    hdr = "  ".join("%04X" % a for a in addrs)
    print("%s  %s   (%s)" % (os.path.join(root, pattern), hdr,
                             "8.8 fixed" if fixed else "raw"))
    prev = None
    for frame, tag, data in rows:
        cells = []
        for a in addrs:
            v = int.from_bytes(data[a:a + 2], "little")
            cells.append("%6d/%8.2f" % (v, v / 256.0) if fixed else "%6d" % v)
        delta = ""
        if prev is not None:
            d = [int.from_bytes(data[a:a + 2], "little") - prev[i] for i, a in enumerate(addrs)]
            delta = "  d=" + "/".join("%+d" % x for x in d)
        print("  f=%-6s %-8s %s%s" % (frame, tag, "  ".join(cells), delta))
        prev = [int.from_bytes(data[a:a + 2], "little") for a in addrs]
    return 0


if __name__ == "__main__":
    main(sys.argv[1:])
