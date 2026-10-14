#!/usr/bin/env python3
"""Summarize a SNESRECOMP_PRESENT_LOG csv.

Usage: python tools/present_curve.py <presents.csv>

Prints when the screen first leaves black, every distinct image (with its
frame), and the last frames -- enough to tell "boots to a title screen" from
"one black screen for 30 seconds" without opening a single picture.
"""
import csv
import sys


def main(argv):
    path = argv[0]
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.append((int(r["present"]), int(r["frame"]), float(r["luma"]), r["crc32"]))
    print("presents: %d (frames %d..%d)" % (len(rows), rows[0][1], rows[-1][1]))
    first_lit = next((r for r in rows if r[2] > 0.5), None)
    print("first non-black present: %s" % (("present=%d frame=%d luma=%.3f" % first_lit[:3])
                                           if first_lit else "NONE (all black)"))
    seq = []
    prev = None
    for present, frame, luma, crc in rows:
        if crc != prev:
            seq.append((present, frame, luma, crc))
            prev = crc
    print("distinct images: %d" % len(seq))
    for present, frame, luma, crc in seq[:40]:
        print("  f=%-6d luma=%-7.3f %s" % (frame, luma, crc))
    if len(seq) > 40:
        print("  ... (%d more)" % (len(seq) - 40))
    lumas = [r[2] for r in rows]
    print("luma min=%.3f max=%.3f mean=%.3f" % (min(lumas), max(lumas), sum(lumas) / len(lumas)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
