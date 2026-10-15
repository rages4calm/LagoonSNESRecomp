"""Compare private camera runs and verify resolved-position/queue projections.

Use the stop/turn route for --baseline; the projectile fixture intentionally
seeds one ordinary projectile and is not a campaign/magic acquisition test.
"""
import argparse
import json
from pathlib import Path


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--normal', required=True, type=Path)
    p.add_argument('--wide', required=True, type=Path)
    p.add_argument('--baseline', type=Path)
    p.add_argument('--out', required=True, type=Path)
    a = p.parse_args()
    result = dict(tags=0, errors=[], offsets=[], projectile_witnesses=0,
                  player_world_differences=[], executable_sha256=None)
    receipt = a.normal / 'receipt.json'
    if receipt.exists():
        result['executable_sha256'] = json.loads(receipt.read_text())['exe_sha256']
    left, right = a.normal / 'dumps', a.wide / 'dumps'
    tags = {x.name[:-9] for x in left.glob('*.wram.bin')}
    if not tags or tags != {x.name[:-9] for x in right.glob('*.wram.bin')}:
        raise SystemExit('Missing or unequal dump tags')
    for tag in sorted(tags):
        result['tags'] += 1
        for extension in ('wram.bin', 'vram.bin', 'oam.bin', 'cgram.bin', 'regs.json', 'fb.bmp'):
            if (left / (tag+'.'+extension)).read_bytes() != (right / (tag+'.'+extension)).read_bytes():
                result['errors'].append([tag, extension])
        info = [json.loads((d / (tag+'.info.json')).read_text()) for d in (left, right)]
        for field in ('frame', 'master_clock', 'hpos', 'vpos'):
            if info[0][field] != info[1][field]:
                result['errors'].append([tag, field])
        ram = (left / (tag+'.wram.bin')).read_bytes()
        word = lambda offset: int.from_bytes(ram[offset:offset+2], 'little')
        if a.baseline:
            old = (a.baseline / 'dumps' / (tag+'.wram.bin')).read_bytes()
            if old[0x502:0x506] != ram[0x502:0x506]:
                result['player_world_differences'].append(tag)
        if '-held' in tag or '-stop-' in tag:
            offset = [word(0x502)-word(0x480), word(0x504)-word(0x482)]
            result['offsets'].append([tag, *offset])
            if offset != [128, 112]:
                result['errors'].append([tag, 'interior_camera_offset', offset])
        if tag.startswith('projectile-'):
            for i in range(ram[0x48c]):
                q = 0xec0+i*6
                if ram[q+4] != 4:
                    continue
                index = word(q)
                expected = ((word(0xd41+index)-word(0x480)) & 65535,
                            (word(0xd43+index)-word(0x482)) & 65535)
                if (word(0xd49+index), word(0xd4b+index)) != expected or word(q+2) != expected[1]:
                    result['errors'].append([tag, 'projectile_screen_or_sort_key'])
                result['projectile_witnesses'] += 1
    a.out.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({key: result[key] for key in ('tags', 'projectile_witnesses', 'errors', 'player_world_differences')}))
    if result['errors'] or result['player_world_differences']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
