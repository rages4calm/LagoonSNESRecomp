"""Count visible margin pixels matching actual OAM/VRAM/CGRAM at one capture.

Supply only independently validated actor OAM slots, excluding parked/UI slots.
This is a visibility/color witness, not an assertion that every source pixel
must be visible: foregrounds, windows and earlier objects can obscure pixels.
The bounded Lagoon checks use OBSEL2, brightness15 and no color math.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dump', required=True, type=Path, help='Dump prefix, without .oam.bin')
    parser.add_argument('--image', required=True, type=Path)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--slots', help='Comma-separated independently validated actor slots')
    group.add_argument('--cells', type=Path, help='Actual emitted far-cell TSV diagnostic')
    parser.add_argument('--cell-frame', type=int, help='Exact even OAM snapshot frame held by this capture')
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    read = lambda suffix: Path(str(args.dump) + suffix).read_bytes()
    regs = json.loads(read('.regs.json'))
    assert regs['obsel'] == 2 and regs['inidisp'] == 15 and regs['cgadsub'] == 0
    oam = read('.oam.bin')
    vram = struct.unpack('<32768H', read('.vram.bin'))
    palette = struct.unpack('<256H', read('.cgram.bin'))
    image = Image.open(args.image).convert('RGB')
    extra = (image.width - 256) // 2
    expected = {}
    slots = sorted(set(map(int, args.slots.split(',')))) if args.slots else []
    cells = []
    if args.cells:
        assert args.cell_frame is not None
        for row in csv.DictReader(args.cells.open(), delimiter='\t'):
            if row['host_frame'] != str(args.cell_frame): continue
            cells.append((int(row['x']), int(row['y']), int(row['tile'],16), int(row['size'])))
        assert cells, 'No actual emitted cells at requested snapshot'
    assert all(0 <= s < 128 for s in slots)
    for slot in slots:
        x, y, tile = struct.unpack_from('<BBH', oam, slot * 4)
        high = (oam[512 + slot // 4] >> ((slot % 4) * 2)) & 3
        x |= (high & 1) << 8
        if x >= 384:
            x -= 512  # bounded ordinary native-active range
        if y >= 224:
            y -= 256  # OAM Y wraps; validated cells may straddle output top
        size = 16 if high & 2 else 8
        cells.append((x, y, tile, size))
    for x, y, tile, size in cells:
        for py in range(size):
            for px in range(size):
                xx, yy = x + px, y + py
                if 0 <= xx < 256 or not (-extra <= xx < 256 + extra and 0 <= yy < image.height):
                    continue
                sx = size - 1 - px if tile & 0x4000 else px
                sy = size - 1 - py if tile & 0x8000 else py
                char = ((((tile & 255) >> 4) + (sy >> 3)) << 4) | (((tile & 15) + (sx >> 3)) & 15)
                addr = ((0x5000 if tile & 256 else 0x4000) + char * 16 + (sy & 7)) & 32767
                if 0x4000 <= addr < 0x4240:
                    continue
                planes = vram[addr] | vram[(addr + 8) & 32767] << 16
                bit = 7 - (sx & 7)
                color = ((planes >> bit) & 1) | ((planes >> (bit + 7)) & 2) | ((planes >> (bit + 14)) & 4) | ((planes >> (bit + 21)) & 8)
                if not color:
                    continue
                rgb15 = palette[128 + ((tile >> 9) & 7) * 16 + color]
                rgb = tuple((((rgb15 >> shift) & 31) << 3) | (((rgb15 >> shift) & 31) >> 2) for shift in (0, 5, 10))
                expected.setdefault((xx, yy), rgb)  # earlier OAM slot wins
    coordinate_digest=hashlib.sha256(json.dumps(sorted(map(list,expected),key=lambda q:(q[1],q[0])),separators=(',',':')).encode()).hexdigest()
    result = dict(frame=regs['frame'], source_pixel_coordinates_sha256=coordinate_digest, slots=slots, cell_frame=args.cell_frame, emitted_cells=len(cells), source_opaque=dict(left=0, right=0), visible_exact_color=dict(left=0, right=0))
    for (x, y), color in expected.items():
        side = 'left' if x < 0 else 'right'
        result['source_opaque'][side] += 1
        result['visible_exact_color'][side] += image.getpixel((extra + x, y)) == color
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
