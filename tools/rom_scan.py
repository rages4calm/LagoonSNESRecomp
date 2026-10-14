#!/usr/bin/env python3
"""Find byte sequences in the ROM (or any dump) and report them as bus addresses.

  python tools/rom_scan.py --seq 28,17,28,27
  python tools/rom_scan.py --seq 28,17,28,28 --width 2
  python tools/rom_scan.py --bytes 1c,11,1c,1b --limit 20
  python tools/rom_scan.py --seq 10,5 --file build/dumps-qreg-aot/town.wram.bin

Why this exists: a QoL feature is usually a number the game already stores. The
owner's measurements are the fastest evidence available, so the numbers get
searched for before anybody theorises about them. A four-direction table like
sword reach (28,17,28,27 pixels for right,down,left,up) should appear as one
contiguous run, and if it does not, that is itself the finding -- the value is
computed, or split across tables, and a mod must not pretend otherwise.

Matches are reported at the file offset, the LoROM bus address a read would use,
and the same offset mirrored in the $80-$FF banks, because the game can reach
either and the code tells you which one it actually uses. Encoding matters: the
same run searched as bytes (`--width 1`) and as 16-bit little-endian words
(`--width 2`) is two different hypotheses, so the tool does not guess for you.
"""
import os
import sys

BANK_SHIFT = 15          # LoROM: 0x8000 bytes of each bank are mapped
BANK_MASK = 0x7FFF


def load(path):
    with open(path, "rb") as fh:
        return fh.read()


def bus_of(offset):
    """LoROM bus address(es) a file offset is readable at."""
    bank = offset >> BANK_SHIFT
    addr = 0x8000 | (offset & BANK_MASK)
    return "%02X:%04X" % (bank & 0x1F, addr), "%02X:%04X" % (0x80 | (bank & 0x1F), addr)


def context(data, off, n):
    lo = max(0, off - n)
    hi = min(len(data), off + n)
    return " ".join("%02X" % b for b in data[lo:hi])


def find_all(data, needle):
    out, i = [], data.find(needle)
    while i != -1:
        out.append(i)
        i = data.find(needle, i + 1)
    return out


def main(argv):
    seq, width, limit, path = None, 1, 50, None
    as_bytes, use_hex = None, False
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--seq":
            seq = [int(v) for v in argv[i + 1].split(",")]; i += 2
        elif a == "--bytes":
            as_bytes = [int(v, 16) for v in argv[i + 1].split(",")]; i += 2
        elif a == "--width":
            width = int(argv[i + 1]); i += 2
        elif a == "--limit":
            limit = int(argv[i + 1]); i += 2
        elif a == "--file":
            path, use_hex = argv[i + 1], True; i += 2
        else:
            sys.exit("unknown argument: " + a)
    values = seq if seq is not None else as_bytes
    if values is None:
        sys.exit(__doc__)
    if not use_hex and seq is not None and any(v > 0xFF for v in values) and width == 1:
        sys.exit("values above 255 need --width 2 (or --bytes for raw hex)")
    if path is None:
        path = os.environ.get("SNESRECOMP_ROM")
        if not path or not os.path.exists(path):
            sys.exit("no ROM: set SNESRECOMP_ROM or pass --file")
    data = load(path)
    if width == 1:
        needle = bytes(values)
    elif width == 2:
        needle = b"".join(bytes([v & 0xFF, (v >> 8) & 0xFF]) for v in values)
    else:
        sys.exit("--width must be 1 or 2")
    hits = find_all(data, needle)
    label = "u%d" % (8 * width)
    print("%s: %d bytes searched in %s (%d bytes), %d match(es)"
          % (label, len(needle), os.path.basename(path), len(data), len(hits)))
    for off in hits[:limit]:
        lo, mirror = bus_of(off)
        print("  file %06X  bus %s  (mirror %s)  ..%s.." % (off, lo, mirror, context(data, off, 6)))
    if len(hits) > limit:
        print("  ... %d more, raise --limit" % (len(hits) - limit))
    if not hits:
        print("  no match: %s encoded as %s" % (values, label))
    return 0 if hits else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
