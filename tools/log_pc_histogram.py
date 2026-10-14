#!/usr/bin/env python3
"""Analyze a host diagnostic log: where the interpreter is spending time.

Usage: python tools/log_pc_histogram.py <log> [--window N] [--tail]

Prints the histogram of PCs over the last N pctrace lines (the steady state the
guest is stuck in) and a literal tail, so a spin loop is obvious.
"""
import collections
import re
import sys

PC_RE = re.compile(r"pre-runOpcode pc=([0-9a-fA-F]{6})")


def main(argv):
    path = argv[0]
    window = 200000
    if "--window" in argv:
        window = int(argv[argv.index("--window") + 1])
    hist = collections.Counter()
    ops = collections.Counter()
    lines = collections.deque(maxlen=window)
    total = 0
    with open(path, "r", errors="replace") as f:
        for line in f:
            total += 1
            lines.append(line.rstrip())
    for line in lines:
        m = PC_RE.search(line)
        if m:
            hist[m.group(1).upper()] += 1
    print("total log lines: %d (window %d)" % (total, window))
    print("distinct PCs in window: %d" % len(hist))
    for pc, n in hist.most_common(25):
        print("  $%s  %d" % (pc, n))
    if "--tail" in argv:
        print("--- last 40 lines:")
        for line in list(lines)[-40:]:
            print("  " + line)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
