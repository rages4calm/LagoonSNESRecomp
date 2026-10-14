#!/usr/bin/env python3
"""Small 65816 disassembler for reading the ROM while porting.

Usage:
  python tools/dis65816.py <rom> <cpu_pc> [count|--end <cpu_pc>] [--hirom]

  <cpu_pc>   24-bit CPU address in hex, with or without a colon ($00:8148,
             008148, or 8148 for bank 00).
  count      number of instructions (default 40).
  --end      stop at this CPU address instead of counting.
  --hirom    HiROM mapping (default is LoROM).

Addresses are shown as BB:AAAA with the file offset in brackets, and any
absolute operand gets its own offset so a jump target is readable as code.
Mode 00 only: this is a reading aid for a human, not an assembler.
"""
import sys

# mode codes: i=imm8 I=imm16 r=rel8 R=rel16 a=abs A=abs,X B=abs,Y
#             l=abs16 (far addr) L=abs16,X z=zp Z=zp,X Y=zp,Y
#             p=(zp,X) q=(zp),Y j=(abs) J=(abs,X) k=[abs] K=[abs],X
#             s=stack m=imp n=imp(8-bit acc assumed) o=imm8,abs16
MODES = [
    ("brp", "i"), ("ora", "p"), ("mvp", "m"), ("mva", "m"), ("tsb", "z"), ("ora", "z"), ("asl", "z"), ("rmb", "z"),
    ("php", "m"), ("ora", "i"), ("asl", "m"), ("pha", "m"), ("phk", "m"), ("ora", "a"), ("asl", "a"), ("obb", "z"),
    ("bbr", "r"), ("ora", "Z"), ("ora", "z"), ("aso", "Z"), ("tsb", "A"), ("ora", "A"), ("rls", "A"), ("obb", "A"),
    ("plp", "m"), ("eor", "i"), ("inc", "m"), ("wai", "m"), ("cdb", "m"), ("eor", "a"), ("inc", "a"), ("obb", "A"),
    ("bbr", "r"), ("eor", "p"), ("eor", "z"), ("esc", "m"), ("bit", "z"), ("eor", "z"), ("rol", "z"), ("rmb", "z"),
    ("plc", "m"), ("eor", "I"), ("mvp", "m"), ("pl", "m"), ("pwd", "m"), ("eor", "j"), ("rol", "a"), ("obb", "z"),
    ("bbr", "r"), ("eor", "Z"), ("eor", "z"), ("aso", "Z"), ("bit", "A"), ("eor", "A"), ("rls", "A"), ("obb", "A"),
    ("plp", "m"), ("adc", "i"), ("vd", "m"), ("pbk", "m"), ("stp", "m"), ("adc", "a"), ("lac", "a"), ("obb", "A"),
    ("bbr", "r"), ("adc", "p"), ("adc", "z"), ("alc", "z"), ("tsz", "A"), ("adc", "A"), ("tar", "A"), ("obb", "A"),
    ("phy", "m"), ("lda", "i"), ("cdh", "m"), ("pha", "m"), ("xcp", "m"), ("lda", "a"), ("cmp", "a"), ("obb", "z"),
    ("bbr", "r"), ("lda", "p"), ("lda", "z"), ("alr", "z"), ("bit", "z"), ("lda", "z"), ("lan", "z"), ("rmb", "z"),
    ("rty", "m"), ("adc", "I"), ("ad", "m"), ("pla", "m"), ("rtl", "m"), ("lda", "j"), ("adc", "a"), ("obb", "A"),
    ("bbr", "r"), ("lda", "Z"), ("lda", "z"), ("alr", "Z"), ("tsb", "A"), ("lda", "A"), ("aim", "A"), ("obb", "A"),
    ("cpx", "i"), ("sbc", "p"), ("sbc", "z"), ("zn", "m"), ("stz", "z"), ("sbc", "z"), ("sbc", "z"), ("rmb", "z"),
    ("cxf", "m"), ("sbc", "I"), ("xcs", "m"), ("plx", "m"), ("xch", "m"), ("sbc", "a"), ("sbc", "a"), ("obb", "z"),
    ("bbr", "r"), ("sbc", "Z"), ("sbc", "z"), ("sbc", "Z"), ("stz", "A"), ("sbc", "A"), ("sbc", "A"), ("obb", "A"),
    ("inx", "m"), ("jsr", "a"), ("inx", "m"), ("push", "i"), ("bit", "a"), ("and", "a"), ("ea", "a"), ("obb", "A"),
    ("bbr", "r"), ("and", "p"), ("bit", "z"), ("bit", "Z"), ("tsb", "z"), ("and", "z"), ("bit", "A"), ("rmb", "z"),
    ("ei", "m"), ("and", "i"), ("rep", "i"), ("psh", "i"), ("cip", "m"), ("and", "a"), ("cab", "m"), ("obb", "A"),
    ("bbr", "r"), ("and", "Z"), ("and", "z"), ("ane", "Z"), ("bit", "z"), ("and", "z"), ("sha", "Z"), ("obb", "A"),
    ("sty", "z"), ("lda", "p"), ("stx", "z"), ("stz", "z"), ("sty", "A"), ("lda", "A"), ("stx", "A"), ("rmb", "z"),
    ("tsy", "m"), ("lda", "k"), ("taz", "m"), ("tas", "m"), ("sty", "Z"), ("lda", "K"), ("spx", "m"), ("obb", "A"),
    ("bbr", "r"), ("sta", "p"), ("sta", "z"), ("sxo", "z"), ("stt", "z"), ("sta", "z"), ("sxo", "Z"), ("rmb", "z"),
    ("ldy", "i"), ("cmp", "p"), ("tyx", "m"), ("tly", "m"), ("lby", "i"), ("cmp", "z"), ("tcp", "m"), ("rmb", "z"),
    ("pyz", "m"), ("cmp", "i"), ("tns", "m"), ("pky", "i"), ("tyz", "m"), ("cmp", "a"), ("tab", "m"), ("obb", "A"),
    ("bbr", "r"), ("cmp", "Z"), ("cmp", "z"), ("lax", "Z"), ("ldy", "A"), ("cmp", "A"), ("lan", "A"), ("obb", "A"),
    ("cpy", "i"), ("sbc", "p"), ("inc", "z"), ("int", "i"), ("lby", "i"), ("sbc", "z"), ("tac", "m"), ("rmb", "z"),
    ("tya", "m"), ("sbc", "I"), ("txs", "m"), ("txy", "m"), ("stz", "Z"), ("sbc", "A"), ("tcs", "m"), ("obb", "A"),
    ("bbr", "r"), ("sbc", "Z"), ("sbc", "z"), ("axs", "Z"), ("ldy", "z"), ("sbc", "z"), ("sax", "Z"), ("rmb", "z"),
    ("ldx", "i"), ("ldy", "a"), ("ldx", "a"), ("ldy", "z"), ("ldy", "Z"), ("ldx", "a"), ("ldx", "z"), ("rmb", "z"),
    ("tax", "m"), ("txa", "m"), ("txy", "m"), ("txa", "m"), ("txs", "m"), ("tpa", "m"), ("tnx", "m"), ("xba", "m"),
    ("bbr", "r"), ("lda", "Z"), ("lda", "z"), ("ldy", "Z"), ("ldy", "A"), ("lda", "K"), ("lax", "A"), ("xbx", "m"),
]

# operand width per mode code
WIDTH = {"i": 1, "I": 2, "r": 1, "R": 2, "a": 2, "A": 2, "l": 3, "L": 3,
         "z": 1, "Z": 1, "Y": 1, "p": 1, "q": 1, "j": 2, "J": 2, "k": 2,
         "K": 2, "s": 0, "m": 0, "n": 0, "o": 3}
SUFFIX = {
    "a": "abs", "A": "abs,x", "z": "zp", "Z": "zp,x", "Y": "zp,y",
    "p": "(zp,x)", "q": "(zp),y", "j": "(abs)", "J": "(abs,x)",
    "k": "[abs]", "K": "[abs],y", "l": "abs16", "L": "abs16,x",
    "i": "#$", "I": "#$", "o": "#$,abs",
}


def to_offset(cpu, hirom):
    bank = (cpu >> 16) & 0xFF
    addr = cpu & 0xFFFF
    if hirom:
        if bank in (0x00, 0x01) or bank >= 0x80:
            b = bank & 0x7F
            return b * 0x10000 + addr if b < 0x40 else (b - 0x40) * 0x10000 + addr
    return ((bank & 0x7F) * 0x8000) + (addr & 0x7FFF)


def main(argv):
    rom_path = argv[0]
    hirom = "--hirom" in argv
    argv = [a for a in argv if a != "--hirom"]
    start = int(argv[1].replace("$", "").replace(":", ""), 16)
    end = None
    count = 40
    if len(argv) > 2:
        if argv[2] == "--end":
            end = int(argv[3].replace("$", "").replace(":", ""), 16)
        else:
            count = int(argv[2])
    data = open(rom_path, "rb").read()

    pc = start
    out = []
    while True:
        off = to_offset(pc, hirom)
        if off + 8 > len(data):
            out.append("%06X  ?? out of ROM" % pc)
            break
        op = data[off]
        name, mode = MODES[op]
        n = len(mode)
        ops = data[off + 1:off + 1 + n]

        def w(k, base):
            return sum(ops[i + base] << (8 * i) for i in range(WIDTH[k]))

        txt = name
        if not mode:
            pass
        elif mode[0] in "iI":
            txt += " #$%0*X" % (WIDTH[mode[0]] * 2, w(mode[0], 0))
        elif mode[0] == "o":
            txt += " #$%02X,$%04X" % (ops[0], ops[1] | (ops[2] << 8))
        elif mode[0] in "rR":
            rel = ops[0] if WIDTH[mode[0]] == 1 else ops[0] | (ops[1] << 8)
            rel -= 0x100 if WIDTH[mode[0]] == 1 else 0x10000
            tgt = (pc + 1 + n + rel) & 0xFFFFFF
            txt += " $%06X" % tgt
        elif mode[0] in "zZ Ypq":
            v = ops[0]
            txt += " $%02X" % v
            if mode[0] == "Z":
                txt += ",x"
            elif mode[0] == "Y":
                txt += ",y"
            elif mode[0] == "p":
                txt = "%s ($%02X,x)" % (name, v)
            elif mode[0] == "q":
                txt = "%s ($%02X),y" % (name, v)
        else:
            k = mode[0]
            v = w(k, 0)
            if k in "jJkK":
                txt = "%s %s$%04X%s" % (name, "[", v, "]" if k in "kK" else "")
                if k in "J":
                    txt += ",x"
                if k == "K":
                    txt += ",y"
            else:
                txt += " $%04X" % v
                if k in "AL":
                    txt += ",x" if k == "A" else ",x"
                if k == "l":
                    # far address: bank shown as operand bytes
                    txt = "%s $%02X:%04X" % (name, ops[2], v)
        hexb = data[off:off + 1 + n].hex(" ")
        note = ""
        if mode and mode[0] in "aAlLjJkK" and n:
            tgt = None
            if mode[0] == "l":
                tgt = (ops[2] << 16) | w("l", 0)
            elif mode[0] in "ajA":
                tgt = ((pc >> 16) << 16) | w(mode[0], 0)
            if tgt is not None and to_offset(tgt, hirom) + 1 < len(data):
                note = "  ; -> file 0x%X" % to_offset(tgt, hirom)
        out.append("%06X [%06X]  %-14s %s%s" % (pc, off, hexb, txt, note))
        pc += 1 + n
        if end is not None:
            if pc > end:
                break
        elif len(out) >= count:
            break
    print("\n".join(out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
