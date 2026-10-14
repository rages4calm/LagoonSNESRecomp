#!/usr/bin/env python3
"""Find WRAM addresses that change at a fixed rate per frame across a dump series.

  python tools/dump_series.py build/dumps-walkhunt --delta 3 --delta 1
  python tools/dump_series.py build/dumps-walkhunt --delta 3 --subpixel

A framedump series from a route that holds one direction is a differential
instrument: the game's own motion identifies its own variables. Ask which
addresses advance by exactly 3 pixels per frame and the answer is the player's
coordinate; ask for 1 and the answer is whatever follows him. That is how a
variable gets *found* rather than guessed, and it doubles as a check on a
measured number — if nothing moves by 3, the number is wrong or is a computed
result, and a mod built on it would have been wrong too.

Deltas are matched against every consecutive pair, so a variable that only moves
sometimes cannot sneak through: the score is how many intervals hit the rate
exactly. Coordinates are commonly 8.8 fixed-point, where 3 pixels per frame looks
like 768 per frame in a 16-bit word, so `--subpixel` adds that reading. Values
wrap modulo the width, which is what the hardware does.
"""
import json
import os
import sys

WIDTHS = (1, 2)


def series(dump_dir):
    """(tag, frame, wram bytes) for every dump in the directory, in frame order."""
    out = []
    for name in sorted(os.listdir(dump_dir)):
        if not name.endswith(".info.json"):
            continue
        tag = name[:-len(".info.json")]
        wram = os.path.join(dump_dir, tag + ".wram.bin")
        if not os.path.exists(wram):
            continue
        try:
            with open(os.path.join(dump_dir, name)) as fh:
                frame = json.load(fh)["frame"]
        except (OSError, ValueError, KeyError):
            continue
        with open(wram, "rb") as fh:
            out.append((tag, frame, fh.read()))
    out.sort(key=lambda t: t[1])
    return out


def read(buf, off, width):
    return int.from_bytes(buf[off:off + width], "little")


def constant_deltas(s, held, skip):
    """Addresses whose step between dumps is identical and nonzero every interval.

    This is the honest direction of enquiry: instead of asking whether an address
    moves at a rate somebody measured, ask the dumps what their own rate is. The
    caller passes `held`, how many frames of the interval actually had the button
    down, because a route that pumps a button moves only while it is held; the
    pixel rate is the step divided by that. `skip` drops leading intervals, where
    a sprite is still accelerating from a standstill and a constant step is not
    expected yet.
    """
    hits = []
    for width in WIDTHS:
        mod = 1 << (8 * width)
        for off in range(0, len(s[0][2]) - width + 1):
            vals = [read(s[k][2], off, width) for k in range(len(s))]
            steps = [(vals[k] - vals[k - 1]) % mod for k in range(1, len(vals))]
            used = steps[skip:]
            if not used or len(set(used)) != 1 or used[0] == 0:
                continue
            hits.append((off, width, used[0], steps, vals))
    return hits


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    dump_dir, deltas, subpixel, only = argv[0], [], False, []
    constant, held, skip = False, None, 0
    for i, a in enumerate(argv[1:], start=1):
        if a == "--delta":
            deltas.append(int(argv[i + 1]))
        elif a == "--only":
            only.append(argv[i + 1])
        elif a == "--subpixel":
            subpixel = True
        elif a == "--constant":
            constant = True
        elif a == "--held":
            held = int(argv[i + 1])
        elif a == "--skip":
            skip = int(argv[i + 1])
        elif a.startswith("--"):
            sys.exit("unknown flag " + a)
    s = series(dump_dir)
    if only:
        s = [t for t in s if any(t[0].startswith(p) for p in only)]
    if len(s) < 2:
        sys.exit("need at least two dumps with wram: " + dump_dir)
    span = s[-1][1] - s[0][1]
    gaps = [s[k][1] - s[k - 1][1] for k in range(1, len(s))]
    print("%d dumps, frames %d..%d (%d frames), WRAM %d bytes%s, interval %s"
          % (len(s), s[0][1], s[-1][1], span, len(s[0][2]),
             ("  tags: " + ",".join(only)) if only else "",
             "uniform %d" % gaps[0] if len(set(gaps)) == 1 else str(sorted(set(gaps)))))
    if constant:
        h = held if held is not None else gaps[0]
        hits = constant_deltas(s, h, skip)
        print("\nconstant step per interval (held %d frame(s)/interval, first %d "
              "interval(s) ignored): %d address(es)" % (h, skip, len(hits)))
        for off, width, step, steps, vals in sorted(hits):
            per = ("%.2f" % (step / h)) if step % h == 0 else ("~%.2f" % (step / h))
            print("  7E:%04X u%-2d step %5d  -> %s per held frame   %s"
                  % (off, 8 * width, step, per, " ".join(str(v) for v in vals)))
        return 0
    if not deltas:
        sys.exit("pass --delta N (pixels per frame) or --constant")
    rates = list(deltas)
    if subpixel:
        rates += [d * 256 for d in deltas]
    for width in WIDTHS:
        for rate in rates:
            hits = []
            for off in range(0, len(s[0][2]) - width + 1):
                ok = 0
                vals = [read(s[0][2], off, width)]
                for k in range(1, len(s)):
                    df = s[k][1] - s[k - 1][1]
                    want = (rate * df) % (1 << (8 * width))
                    v = read(s[k][2], off, width)
                    # Something that never changes matches a rate of zero
                    # arithmetic everywhere and proves nothing; exclude it.
                    if v == vals[-1] and rate % (1 << (8 * width)) == 0:
                        pass
                    vals.append(v)
                    if (v - vals[-2]) % (1 << (8 * width)) == want:
                        ok += 1
                if ok == len(s) - 1 and len(set(vals)) > 1:
                    hits.append((off, vals))
            if not hits:
                print("\nu%d rate %+d/frame: no address matches every interval"
                      % (8 * width, rate))
                continue
            print("\nu%d rate %+d/frame: %d address(es), all %d intervals"
                  % (8 * width, rate, len(hits), len(s) - 1))
            for off, vals in hits[:14]:
                print("  7E:%04X  %s" % (off, " ".join(str(v) for v in vals)))
    return 0


if __name__ == "__main__":
    main(sys.argv[1:])
