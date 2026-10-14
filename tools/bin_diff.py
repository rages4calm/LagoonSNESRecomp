#!/usr/bin/env python3
"""Byte-diff two binary snapshots (wram/vram/cgram dumps from two builds).

Usage:
  python tools/bin_diff.py build/dumps-aot4/mv-a.wram.bin build/dumps-move/mv-a.wram.bin [--runs 24]

Names the addresses where an AOT build and the interpreter floor disagree, which
is the question a framebuffer checksum cannot answer: the picture is the symptom,
the changed bytes are the mechanism. Prints the first --runs differing runs and a
per-64K-bank histogram of how far apart the two states have drifted.
"""
import sys


def main(argv):
    runs = 24
    pos = [a for a in argv if not a.startswith('--')]
    if '--runs' in argv:
        runs = int(argv[argv.index('--runs') + 1])
        pos = [a for a in pos if a != str(runs)]
    if len(pos) != 2:
        sys.exit(__doc__)
    a, b = (open(p, 'rb').read() for p in pos)
    n = min(len(a), len(b))
    if len(a) != len(b):
        print(f"size differs: {len(a)} vs {len(b)}")
    diff = [i for i in range(n) if a[i] != b[i]]
    print(f"{len(diff)} of {n} bytes differ ({100.0*len(diff)/max(n,1):.3f}%)")
    if not diff:
        return 0
    # Runs of consecutive differences (gap <= 8 merged) so the output reads as
    # regions rather than a byte list.
    out, start, last = [], None, None
    for i in diff:
        if start is None:
            start = last = i
            continue
        if i - last <= 8:
            last = i
            continue
        out.append((start, last))
        start = last = i
    out.append((start, last))
    print(f"{len(out)} differing regions; first {min(runs, len(out))}:")
    for s, e in out[:runs]:
        print(f"  ${s:06X}-${e:06X} ({e-s+1}B)  "
              f"{' '.join(f'{x:02x}' for x in a[s:min(e, s+11)+1])} -> "
              f"{' '.join(f'{x:02x}' for x in b[s:min(e, s+11)+1])}")
    hist = {}
    for i in diff:
        hist[i >> 12] = hist.get(i >> 12, 0) + 1
    print("per-4K-page counts: " +
          ' '.join(f"{p:02X}h:{c}" for p, c in sorted(hist.items(), key=lambda kv: -kv[1])[:16]))
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
