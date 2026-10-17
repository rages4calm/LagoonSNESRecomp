"""Read-only Lagoon USA room/door/entity catalogue; emits metadata, never assets.

Room count is the 75-word table05:8059 ending at05:80EF. Formats are
source-proven at018B84 (room),01C87B (doors),018C54 (9-byte entities).
Names are deliberately absent: a resource grouping is not an observed location.
"""
import sys
from pathlib import Path
sys.path = [p for p in sys.path if Path(p or '.').resolve() != Path(__file__).resolve().parent]
import argparse
import hashlib
import json

ROM_SHA = '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'


def catalogue(rom):
    if hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError('Requires original unheadered Lagoon USA ROM identity')
    def offset(addr):
        if addr & 0xffff < 0x8000:
            raise ValueError(f'Invalid LoROM pointer {addr:06x}')
        return ((addr >> 16) & 0x7f) * 0x8000 + (addr & 0x7fff)
    def raw(addr, count):
        at = offset(addr)
        data = rom[at:at + count]
        if len(data) != count:
            raise ValueError('Out-of-ROM read')
        return data
    def word(addr):
        return int.from_bytes(raw(addr, 2), 'little')
    rooms = []
    for room in range(75):
        rec = raw(0x05b2df + room * 0x38, 0x38)
        w = lambda off: int.from_bytes(rec[off:off + 2], 'little')
        p = lambda off: int.from_bytes(rec[off:off + 3], 'little')
        door_ptr = 0x050000 | word(0x058059 + room * 2)
        exits = []
        for n in range(256):
            at = door_ptr + n * 11
            if word(at) == 0xffff:
                break
            d = raw(at, 11)
            dw = lambda off: int.from_bytes(d[off:off + 2], 'little')
            if d[8] >= 75:
                raise ValueError(f'Invalid door destination at {at:06x}')
            exits.append(dict(address=f'{at:06x}', cell=[dw(0), dw(2)],
                trigger_pixel_box=[dw(0)*32, dw(2)*32+1, dw(0)*32+31, dw(2)*32+32],
                destination_room=d[8], destination_xy=[dw(4), dw(6)],
                facing=d[9], flags=d[10]))
        else:
            raise ValueError('Missing door terminator')
        entities = []
        entity_ptr = p(0x2b)  # DP7B - DP50
        for n in range(32):
            at = entity_ptr + n*9
            e = raw(at, 9)
            if e[0] == 0xff:
                break
            entities.append(dict(address=f'{at:06x}', type=e[0], mode=e[1],
                xy=[int.from_bytes(e[2:4], 'little'), int.from_bytes(e[4:6], 'little')],
                facing=e[6], flags=e[7], parameter=e[8]))
        else:
            raise ValueError('Missing entity terminator')
        rooms.append(dict(id=room, hex=f'{room:02x}', dimensions=[w(3)*8,w(5)*8],
            bg_source=f'{p(7):06x}', obj_source=f'{p(13):06x}',
            bg1_definitions=f'{p(25):06x}', bg2_definitions=f'{p(37):06x}',
            entity_source=f'{entity_ptr:06x}', exits=exits, entities=entities,
            bosses=[e for e in entities if e['type']==3]))
    for room in rooms:
        room['incoming'] = [dict(source_room=source['id'],
            descriptor=f"{int(exit['address'],16)+4:06x}",
            xy=exit['destination_xy'], facing=exit['facing'], flags=exit['flags'])
            for source in rooms for exit in source['exits']
            if exit['destination_room'] == room['id']]
        # Boss completion is room-indexed, not mode-indexed:01B838 ->01CD68.
        if 0x40 <= room['id'] <= 0x4a:
            bit = room['id'] - 0x40
            room['boss_completion_flag'] = dict(address=f'{0x4e0+bit//8:04x}',
                mask=f'{0x80>>(bit%8):02x}', source='01b838/01cd68/01d3cd')
    return dict(rom_sha256=ROM_SHA, source_routines=['018b84','01c87b','018c54'],
        limitations=['Static metadata, not runtime entry or progression qualification.',
            'Doors may be unavailable behind collision/story flags; script warps are separate.',
            'Entity records may be enabled/removed by room scripts.'], rooms=rooms)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--rom', required=True, type=Path)
    ap.add_argument('--out', required=True, type=Path)
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError('Refusing to overwrite existing report')
    result = catalogue(args.rom.read_bytes())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2), encoding='utf-8')
    for r in result['rooms']:
        print(f"{r['hex']} {r['dimensions']} BG{r['bg_source']} OBJ{r['obj_source']} "
              f"boss{[(e['mode'],e['xy']) for e in r['bosses']]} "
              f"doors{[(e['cell'],format(e['destination_room'],'02x'),e['destination_xy']) for e in r['exits']]}")


if __name__ == '__main__':
    main()
