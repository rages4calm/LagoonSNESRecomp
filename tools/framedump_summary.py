#!/usr/bin/env python3
"""Summarize a host framedump directory: WRAM crc per frame + first change points.

Usage: python tools/framedump_summary.py <dir> [more dirs...]
"""
import glob
import json
import os
import sys


def main(paths):
    for root in paths:
        files = sorted(glob.glob(os.path.join(root, "frame_*.json")))
        rows = []
        for p in files:
            with open(p) as f:
                d = json.load(f)
            rows.append((d["frame"], d["crc32_wram"]))
        print("== %s: %d frame dumps" % (root, len(rows)))
        if not rows:
            continue
        changes = []
        prev = None
        for frame, crc in rows:
            if crc != prev:
                changes.append((frame, crc))
                prev = crc
        print("   distinct WRAM states: %d" % len(changes))
        for frame, crc in changes[:40]:
            print("   f=%-6d %s" % (frame, crc))
        if len(changes) > 40:
            print("   ... (%d more)" % (len(changes) - 40))


if __name__ == "__main__":
    main(sys.argv[1:] or ["build/shots"])
