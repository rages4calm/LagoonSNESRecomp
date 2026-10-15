"""Report observed player damage, recovery and death transitions from replay dumps.

A damage event requires an actual HP decrease. Setup edits are described in the
report; no result is inferred from a route name or a fading/removed sprite.
"""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--setup', required=True)
    args = parser.parse_args()
    rows = []
    for path in (args.run/'dumps').glob('*.wram.bin'):
        tag = path.name.removesuffix('.wram.bin')
        info = json.loads((path.parent/(tag+'.info.json')).read_text())
        ram = path.read_bytes()
        word = lambda offset: int.from_bytes(ram[offset:offset+2], 'little')
        signed = lambda offset: int.from_bytes(ram[offset:offset+2], 'little', signed=True)
        rows.append(dict(tag=tag, frame=info['frame'], room=ram[0x48E],
                         x=word(0x502), y=word(0x504), vx=signed(0x506), vy=signed(0x508),
                         hp=word(0x520), flags=word(0x500), hit_counter=ram[0x512],
                         hit_direction=ram[0x513], sprite_state=ram[0x51B],
                         level=ram[0x52C], gold=word(0x528), exp=word(0x52A),
                         inidisp=json.loads((path.parent/(tag+'.regs.json')).read_text())['inidisp']))
    rows.sort(key=lambda row: (row['frame'], row['tag']))
    events = []
    for old, new in zip(rows, rows[1:]):
        changes = {}
        if old['hp'] != new['hp']:
            changes['hp'] = [old['hp'], new['hp']]
        for name in ('flags', 'hit_counter', 'vx', 'vy'):
            if old[name] != new[name]:
                changes[name] = [old[name], new[name]]
        if changes and (old['tag'].startswith('damage') or new['tag'].startswith('damage')):
            events.append(dict(from_tag=old['tag'], to_tag=new['tag'], frame=new['frame'], changes=changes))
    result = dict(setup=args.setup, snapshots=len(rows),
                  hp_decreases=[e for e in events if 'hp' in e['changes'] and e['changes']['hp'][1]<e['changes']['hp'][0]],
                  zero_hp_observed=any(row['hp']==0 and row['room']==3 for row in rows),
                  events=events, rows=rows)
    if args.out:
        args.out.write_text(json.dumps(result, indent=2))
    print(json.dumps({key:value for key,value in result.items() if key not in ('rows','events')}, indent=2))


if __name__ == '__main__':
    main()
