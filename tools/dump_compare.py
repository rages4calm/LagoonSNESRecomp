#!/usr/bin/env python3
"""Compare framedump sets between two runs, byte for byte.

Usage:
  python tools/dump_compare.py build/dumps-aot2 build/dumps-move

Prints one line per shared dump tag: the frame, whether the framebuffer, WRAM,
the register snapshot and the guest clock agree, and how many bytes differ when
they do not. This is the replay comparison the promotion loop asks for — a
screenshot can look identical while WRAM has quietly diverged, and a CRC catches
both.

The clock column exists because a promotion can be semantically perfect and
still be wrong. Ahead-of-time blocks carry one static cycle cost per block,
while the interpreter charges address-transit penalties as they happen, so a
compiled routine can leave `master_clock` a few dots away from the floor on
every frame it runs. Nothing visible changes for thousands of frames; then a
timing-sensitive scene amplifies it and the replay forks. Comparing only
picture and memory lets that through.
"""
import json
import os
import sys
import zlib


def crc(path):
    with open(path, "rb") as fh:
        return format(zlib.crc32(fh.read()) & 0xFFFFFFFF, "08x")


CLOCK_FIELDS = ("vpos", "hpos", "master_clock")


def clock(path):
    """The guest's position in the frame at this dump, or None if unavailable."""
    try:
        with open(path) as fh:
            info = json.load(fh)
    except (OSError, ValueError):
        return None
    if not all(k in info for k in CLOCK_FIELDS):
        return None
    return tuple(info[k] for k in CLOCK_FIELDS)


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
    if not left and not right:
        # A route whose script has no `dump` lines is not a failed comparison;
        # report it that way so a parity run does not read as a regression.
        print(f"no framedumps to compare: {argv[0]} and {argv[1]} share no dump "
              f"tags (pass a pad script with `dump <tag>` lines to get any)")
        return 3
    missing = sorted(set(left) ^ set(right))
    bad = len(missing)
    for t in missing:
        print(f"{t}: missing dump tag on {'right' if t in left else 'left'}")
    for t in shared:
        rows = []
        for suffix in (".fb.bmp", ".wram.bin", ".regs.json", ".vram.bin", ".oam.bin", ".cgram.bin"):
            a, b = left[t][:-len(".fb.bmp")] + suffix, right[t][:-len(".fb.bmp")] + suffix
            if not (os.path.exists(a) and os.path.exists(b)):
                rows.append(f"{suffix[1:]}=MISSING")
                bad += 1
                continue
            same = crc(a) == crc(b)
            rows.append(f"{suffix[1:]}={'ok' if same else 'DIFF(' + crc(a) + '/' + crc(b) + ')'}")
            if not same:
                bad += 1
        stem = left[t][:-len(".fb.bmp")]
        ca = clock(stem + ".info.json")
        cb = clock(right[t][:-len(".fb.bmp")] + ".info.json")
        if ca is None or cb is None:
            rows.append("clock=MISSING")
            bad += 1
        elif ca == cb:
            rows.append("clock=ok")
        else:
            rows.append("clock=DIFF(mclk %d/%d delta=%d)"
                        % (ca[2], cb[2], cb[2] - ca[2]))
            bad += 1
        print(f"{t:8s} {'  '.join(rows)}")
    print(f"\n{len(shared)} tags compared, {bad} component mismatches"
          + (" — replay matches" if bad == 0 else " — DIVERGENCE"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
