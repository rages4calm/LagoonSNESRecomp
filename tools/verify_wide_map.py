"""Audit the read-only $01:8311 widescreen map resolver against guest VRAM.

Inputs stay private. This checks interior native tiles, not a whole-game or
off-screen-object qualification. No ROM bytes are written to the report.
"""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', required=True, type=Path)
    p.add_argument('--dumps', required=True, type=Path)
    p.add_argument('--out', required=True, type=Path)
    args = p.parse_args()
    rom = args.rom.read_bytes()
    reports = []
    for path in sorted(args.dumps.glob('*.wram.bin')):
        ram = path.read_bytes()
        vp = path.with_name(path.name.replace('.wram.bin', '.vram.bin'))
        if not vp.exists():
            continue
        vram = vp.read_bytes()
        word = lambda a: int.from_bytes(ram[a:a + 2], 'little')
        pointer = lambda a: int.from_bytes(ram[a:a + 3], 'little')
        def byte(a):
            bank, off = (a >> 16) & 255, a & 65535
            if bank in (0x7e, 0x7f):
                return ram[(bank - 0x7e) * 65536 + off]
            if (bank & 0x7f) < 32 and off >= 32768:
                return rom[(bank & 31) * 32768 + (off & 32767)]
            raise ValueError(f'unsupported pointer {a:06x}')
        if ram[0x50] != 0 or ram[0x500] & 0xc0 != 0xc0:
            continue
        for layer, mp, df, base in [(0, 0x88, 0x69, 0x7000), (1, 0x8b, 0x75, 0x7800)]:
            checked = 0
            mismatches = []
            # Exclude the streaming overhang, which may already hold the next
            # strip at a checkpoint rather than the current displayed edge.
            for ty in range(word(0x482 + layer * 4) // 8 + 1,
                            word(0x482 + layer * 4) // 8 + 25):
                for tx in range(word(0x480 + layer * 4) // 8 + 1,
                                word(0x480 + layer * 4) // 8 + 31):
                    if tx < 0 or ty < 0 or tx >= 256 or ty >= 256:
                        continue
                    meta = byte(pointer(mp) + (ty // 2) * 128 + tx // 2)
                    a = pointer(df) + meta * 8 + (tx % 2) * 2 + (ty % 2) * 4
                    tile = byte(a) | byte(a + 1) << 8
                    if layer:
                        tile &= 0xdfff
                    va = base + ((tx & 32) << 5) + (ty & 31) * 32 + (tx & 31)
                    actual = int.from_bytes(vram[va * 2:va * 2 + 2], 'little')
                    checked += 1
                    if tile != actual:
                        mismatches.append(dict(x=tx, y=ty, expected=tile, actual=actual))
            reports.append(dict(tag=path.name.removesuffix('.wram.bin'), layer=layer,
                                checked=checked, mismatch_count=len(mismatches),
                                first_mismatches=mismatches[:8]))
    result = dict(rom_sha256=hashlib.sha256(rom).hexdigest(), snapshots=reports,
                  checked=sum(r['checked'] for r in reports),
                  mismatches=sum(r['mismatch_count'] for r in reports))
    args.out.write_text(json.dumps(result, indent=2))
    print(json.dumps({k: result[k] for k in ('checked', 'mismatches')}))
    if not reports or result['mismatches']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
