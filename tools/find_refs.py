#!/usr/bin/env python3
"""Find every ROM reference to a CPU address (who calls this dispatcher?).

Usage:
  python tools/find_refs.py 80e6 df3d c654 8106 [--bank 02]

Scans the whole ROM for JSR abs, JMP abs, JSL abs and JML abs forms of each
address. This is how a dispatch site's semantics get decided from evidence: a
site entered by JSR and left by RTS is a tail (`ptrtail`); a site entered by
JSL whose handlers RTL back into the middle of the trampoline is a pointer call
with an explicit `return:`/`frame:3`; a site no code references at all is a
vector or a data misread, and needs a different answer.
"""
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROM = os.environ.get("SNESRECOMP_ROM") or os.path.join(os.path.dirname(REPO), "Lagoon (USA).sfc")


def off(bank, addr16):
    return (addr16 & 0x7FFF) | (bank & 0x7F) * 0x8000


def main(argv):
    if not argv:
        sys.exit(__doc__)
    rom = open(ROM, "rb").read()
    targets = []
    for a in argv:
        v = int(a.replace("$", ""), 16)
        targets.append(v if v > 0xFFFF else (0 << 16) | v)
    for t in targets:
        a16, bk = t & 0xFFFF, (t >> 16) & 0xFF
        found = []
        for bank in range(0x80):
            base = off(bank, 0)
            chunk = rom[base:base + 0x8000]
            if len(chunk) < 4:
                continue
            i = 0
            while True:
                i = chunk.find(bytes([a16 & 0xFF, a16 >> 8]), i)
                if i < 0:
                    break
                pc = (bank << 16) | (0x8000 + i - 0x8000)
                op = chunk[i - 1] if i else None
                op2 = chunk[i - 2] if i >= 2 else None
                if op == 0x22 and chunk[i + 2] == bk:      # JSL abs.l
                    found.append((("JSL $%02X:%04X" % (bk, a16)), (bank << 16) | i - 2))
                elif op in (0x20, 0x4C, 0xDC, 0x6C) or op2 in (0x22,):
                    found.append((("op%02X @bank%02X" % (op if op else op2, bank)), (bank << 16) | i - 1))
                i += 1
        print(f"$%02X:%04X  candidates:" % (bk, a16))
        for kind, pc in found[:14]:
            print(f"   {kind:24s} at ${pc >> 16:02X}:{pc & 0xFFFF:04X}")
        if not found:
            print("   (none)")


if __name__ == "__main__":
    main(sys.argv[1:])
