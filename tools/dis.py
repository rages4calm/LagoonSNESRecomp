#!/usr/bin/env python3
"""Disassemble Lagoon ROM ranges with the framework's own 65816 decoder.

Usage:
  python tools/dis.py <cpu_pc> [count] [--end <cpu_pc>] [--m 0|1] [--x 0|1] [--rom path]

  <cpu_pc>   24-bit CPU address: $00:8148, 008148, or 8148 (bank 00).
  count      instructions to print (default 40).

Uses snesrecomp/recompiler/snes65816.py and tracks linear REP/SEP width changes.
This is not a control-flow analysis: use explicit entry widths and bounded
ranges, especially around PLP, XCE, branches and embedded data.
"""
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(REPO, "snesrecomp", "recompiler"))
import snes65816 as sn  # noqa: E402

# The ROM is copyrighted and lives outside the repo; env.ps1 at the workspace
# root exports SNESRECOMP_ROM, which is the first place this looks.
DEFAULT_ROM = os.environ.get("SNESRECOMP_ROM") or os.path.join(
    os.path.dirname(REPO), "Lagoon (USA).sfc")


def main(argv):
    rom = DEFAULT_ROM
    if "--rom" in argv:
        i = argv.index("--rom")
        rom = argv[i + 1]
        del argv[i:i + 2]
    m = x = 1
    for flag in ("--m", "--x"):
        if flag in argv:
            i = argv.index(flag)
            val = int(argv[i + 1])
            if flag == "--m":
                m = val
            else:
                x = val
            del argv[i:i + 2]
    end = None
    if "--end" in argv:
        i = argv.index("--end")
        end = int(argv[i + 1].replace("$", "").replace(":", ""), 16)
        del argv[i:i + 2]
    argv = [a for a in argv if not a.startswith("--")]
    start = int(argv[0].replace("$", "").replace(":", ""), 16)
    count = int(argv[1]) if len(argv) > 1 else 40

    sn.set_rom_mapping("lorom")
    data = sn.load_rom(rom)
    bank, pc = (start >> 16) & 0xFF, start & 0xFFFF
    off = sn.rom_offset(bank, pc)
    out = []
    printed = 0
    while True:
        insn = sn.decode_insn(data, off, pc, bank, m, x)
        if insn is None:
            out.append("%02X:%04X [%06X]  db $%02X            ; (undecodable)"
                       % (bank, pc, off, data[off]))
            off += 1
            pc = (pc + 1) & 0xFFFF
            printed += 1
        else:
            M = sn
            mode = insn.mode
            o = insn.operand
            if mode in (M.IMP, M.ACC):
                arg = ""
            elif mode == M.IMM:
                arg = "#$%X" % o if insn.length == 2 else "#$%04X" % o
            elif mode in (M.DP, M.DP_X, M.DP_Y, M.INDIR_DPX, M.DP_INDIR,
                          M.INDIR_Y, M.INDIR_L, M.INDIR_LY):
                arg = "$%02X" % o
            elif mode == M.STK:
                arg = "$%02X,S" % o
            elif mode == M.STK_IY:
                arg = "($%02X,S),Y" % o
            elif mode in (M.LONG, M.LONG_X):
                arg = "$%06X" % o
            else:
                arg = "$%04X" % o
            suffix = {M.DP_X: ",x", M.ABS_X: ",x", M.LONG_X: ",x",
                      M.DP_Y: ",y", M.ABS_Y: ",y", M.INDIR_Y: ",y",
                      M.INDIR_LY: ",y", M.INDIR_X: ",x"}.get(mode, "")
            wrap = {M.INDIR_DPX: "(%s,x)", M.DP_INDIR: "(%s)", M.INDIR: "(%s)",
                    M.INDIR_Y: "(%s)", M.INDIR_L: "[%s]", M.INDIR_LY: "[%s]"}.get(mode)
            if wrap and mode != M.INDIR_Y and mode != M.INDIR_LY:
                arg = wrap % arg
            text = (insn.mnem + (" " + arg if arg else "")).ljust(20) + suffix
            hexb = data[off:off + insn.length].hex(" ")
            tgt = ""
            if insn.mode in (getattr(sn, "ABS", None), getattr(sn, "ABS_X", None),
                             getattr(sn, "ABS_Y", None)):
                tgt = "  ; -> %02X:%04X" % (bank, insn.operand)
            elif insn.mode in (getattr(sn, "LONG", None), getattr(sn, "LONG_X", None)):
                tgt = "  ; -> %06X" % insn.operand
            elif insn.mode in (getattr(sn, "REL", None), getattr(sn, "REL16", None)):
                tgt = "  ; -> %02X:%04X" % (bank, insn.operand)
            out.append("%02X:%04X [%06X]  %-16s %s%s" % (bank, pc, off, hexb, text, tgt))
            if insn.mnem == 'REP':
                if o & 0x20: m = 0
                if o & 0x10: x = 0
            elif insn.mnem == 'SEP':
                if o & 0x20: m = 1
                if o & 0x10: x = 1
            off += insn.length
            pc = (pc + insn.length) & 0xFFFF
            printed += 1
        if pc == 0:
            bank += 1
            off = sn.rom_offset(bank, pc)
        if end is not None and ((bank << 16) | pc) > end:
            break
        if printed >= count:
            break
    print("\n".join(out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
