#!/usr/bin/env python3
"""Compare framedump sets between two runs, byte for byte.

Usage:
  python tools/dump_compare.py build/dumps-aot2 build/dumps-move

Prints one line per shared dump tag: the frame, whether the framebuffer, WRAM
and the register snapshot agree, and how many bytes differ when they do not.
This is the replay comparison the promotion loop asks for — a screenshot can
look identical while WRAM has quietly diverged, and a CRC catches both.
"""
import os
import sys
import zlib


def crc(path):
    with open(path, "rb") as fh:
        return format(zlib.crc32(fh.read()) & 0xFFFFFFFF, "08x")


def tags(d):
    out = {}
    for name in sorted(os.listdir(d)):
        if name.endswith(".fb.bmp"):
            out[name[:-len(".fb.bmp")]] = os.path.join(d, name)
    return out


def main(argv):
    if len(argv) != 2:
        sys.exit(__doc__)
    left, right = tags(argv[0]), tags(argv[1])
    shared = sorted(set(left) & set(right))
    if not shared:
        # A route whose script has no `dump` lines is not a failed comparison;
        # report it that way so a parity run does not read as a regression.
        print(f"no framedumps to compare: {argv[0]} and {argv[1]} share no dump "
              f"tags (pass a pad script with `dump <tag>` lines to get any)")
        return 3
    bad = 0
    for t in shared:
        rows = []
        for suffix in (".fb.bmp", ".wram.bin", ".regs.json", ".vram.bin", ".oam.bin", ".cgram.bin"):
            a, b = left[t][:-len(".fb.bmp")] + suffix, right[t][:-len(".fb.bmp")] + suffix
            if not (os.path.exists(a) and os.path.exists(b)):
                continue
            same = crc(a) == crc(b)
            rows.append(f"{suffix[1:]}={'ok' if same else 'DIFF(' + crc(a) + '/' + crc(b) + ')'}")
            if not same:
                bad += 1
        print(f"{t:8s} {'  '.join(rows)}")
    print(f"\n{len(shared)} tags compared, {bad} component mismatches"
          + (" — replay matches" if bad == 0 else " — DIVERGENCE"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
