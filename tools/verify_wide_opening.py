"""Audit the initial fade's actual native center and newly revealed scenery.

Uses one loaded, stationary room source snapshot and independently decodes its
retained maps, VRAM and CGRAM. The actual native field identifies each fade's
brightness; no extra-margin pixels are used to choose that brightness. All
captures and ROM inputs remain private.
"""
import argparse
import csv
import json
import struct
from pathlib import Path
from PIL import Image, ImageChops


def matches(a, b):
    channels = ImageChops.difference(a, b).split()
    maximum = ImageChops.lighter(ImageChops.lighter(channels[0], channels[1]), channels[2])
    return maximum.histogram()[0]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--run', required=True, type=Path)
    p.add_argument('--control', required=True, type=Path)
    p.add_argument('--baseline', required=True, type=Path)
    p.add_argument('--rom', required=True, type=Path)
    p.add_argument('--source-tag', required=True)
    p.add_argument('--out', required=True, type=Path)
    args = p.parse_args()
    sets = [list(csv.DictReader((d/'presents.csv').open()))
            for d in (args.run, args.control, args.baseline)]
    # A final binary binding may capture only the complete fade interval.
    # Match controls by guest frame, never by unrelated presentation indices.
    frame_ids = [r['frame'] for r in sets[0]]
    assert frame_ids and len(frame_ids) == len(set(frame_ids))
    for index in (1, 2):
        by_frame = {r['frame']: r for r in sets[index]}
        assert all(frame in by_frame for frame in frame_ids)
        sets[index] = [by_frame[frame] for frame in frame_ids]
    prefix = args.run/'dumps'/args.source_tag
    read = lambda suffix: Path(str(prefix)+suffix).read_bytes()
    ram, rom = read('.wram.bin'), args.rom.read_bytes()
    vram, palette = struct.unpack('<32768H', read('.vram.bin')), struct.unpack('<256H', read('.cgram.bin'))
    regs = json.loads(read('.regs.json'))
    assert regs['cgadsub'] == 0 and regs['hscroll'][0] == regs['hscroll'][1]
    assert regs['vscroll'][0] == regs['vscroll'][1]
    ptr = lambda a: int.from_bytes(ram[a:a+3], 'little')
    word = lambda a: int.from_bytes(ram[a:a+2], 'little')

    def byte(address):
        bank, offset = address >> 16, address & 65535
        if bank in (126, 127):
            return ram[(bank-126)*65536+offset]
        assert bank & 127 < 32 and offset >= 32768
        return rom[(bank & 31)*32768+(offset & 32767)]

    def pixel(layer, x, y):
        meta = byte(ptr(0x8b if layer else 0x88)+(y >> 4)*128+(x >> 4))
        a = ptr(0x75 if layer else 0x69)+meta*8+((x & 8) >> 2)+((y & 8) >> 1)
        tile = byte(a) | byte(a+1) << 8
        if layer:
            tile &= 0xdfff
        sx, sy = 7-(x & 7) if tile & 0x4000 else x & 7, 7-(y & 7) if tile & 0x8000 else y & 7
        a = (((regs['bg_tile_adr'] >> (layer*4)) & 15) << 12)+(tile & 1023)*16+sy
        planes, bit = vram[a & 32767] | vram[(a+8) & 32767] << 16, 7-sx
        color = ((planes >> bit) & 1) | ((planes >> (bit+7)) & 2) | ((planes >> (bit+14)) & 4) | ((planes >> (bit+21)) & 8)
        return ((12 if tile & 0x2000 else 8)-layer, ((tile >> 10) & 7)*16+color) if color else (0, 0)

    image = lambda folder, row: Image.open(folder/'frames'/('present_%06d.ppm' % int(row['present']))).convert('RGB')
    width = image(args.run, sets[0][0]).width
    extra = (width-256)//2
    cx, cy = word(0x480), word(0x482)
    wx = cx+(((regs['hscroll'][0] & 1023)-cx+512) & 1023)-512
    wy = cy+(((regs['vscroll'][0] & 1023)-cy+512) & 1023)-512+1
    base = Image.new('RGB', (width, 224))
    colors = []
    for y in range(224):
        for x in range(width):
            world_x, world_y = wx+x-extra, wy+y
            assert 0 <= world_x < word(0x53)*8 and 0 <= world_y < word(0x55)*8
            index = max(pixel(0, world_x, world_y), pixel(1, world_x, world_y))[1]
            rgb = palette[index]
            colors.append(tuple((((rgb >> shift) & 31) << 3) | (((rgb >> shift) & 31) >> 2) for shift in (0, 5, 10)))
    base.putdata(colors)
    fades = [base.point([i*brightness//15 for i in range(256)]*3) for brightness in range(16)]
    scene_rows = {int(row['frame']): row for row in csv.DictReader(
        (args.run/'entities.txt.scene.tsv').open(), delimiter='\t')}
    result = dict(presents=len(sets[0]), native_center_mismatches=[], first_native_world=None,
                  first_wide_world=None, newly_wide=[], checked_new_margin_pixels=0, exact_new_margin_pixels=0)
    for row, control, old in zip(*sets):
        frame = int(row['frame'])
        im, native, previous = image(args.run, row), image(args.control, control), image(args.baseline, old)
        actual_native = im.crop((extra, 0, extra+256, 224))
        if actual_native.tobytes() != native.tobytes():
            result['native_center_mismatches'].append(frame)
        # The title card occupies the middle; world imagery reaches this corner.
        scene = scene_rows.get(frame, {})
        if (result['first_native_world'] is None and scene.get('map_ready') == '1'
                and scene.get('bg_ready') == '1' and native.crop((0, 0, 32, 96)).getbbox()):
            result['first_native_world'] = frame
        wide = bool(im.crop((0, 0, extra, 224)).getbbox())
        if wide and result['first_wide_world'] is None:
            result['first_wide_world'] = frame
        if not wide or previous.crop((0, 0, extra, 224)).getbbox():
            continue
        # Exclude HUD rows and allow native sprites to occlude the independent
        # background. A unique overwhelmingly matching brightness is required.
        scores = [matches(f.crop((extra, 0, extra+256, 184)), native.crop((0, 0, 256, 184))) for f in fades]
        brightness = scores.index(max(scores))
        assert scores.count(scores[brightness]) == 1 and scores[brightness] > 256*184*0.9
        checked = exact = 0
        for box in ((0, 0, extra, 224), (extra+256, 0, width, 224)):
            checked += (box[2]-box[0])*224
            exact += matches(im.crop(box), fades[brightness].crop(box))
        result['newly_wide'].append(dict(frame=frame, brightness=brightness, checked=checked, exact=exact))
        result['checked_new_margin_pixels'] += checked
        result['exact_new_margin_pixels'] += exact
    args.out.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: len(v) if isinstance(v, list) else v for k, v in result.items()}))
    if result['native_center_mismatches'] or result['first_native_world'] != result['first_wide_world'] or result['checked_new_margin_pixels'] != result['exact_new_margin_pixels']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
