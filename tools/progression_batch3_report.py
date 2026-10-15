"""Summarize physical WRAM evidence from Lagoon progression replay dumps.

Reports measurements rather than labelling every disappearing sprite a kill.
A reward needs a real player EXP increase; fixtures are explicitly labelled.
"""
import argparse
import json
from pathlib import Path


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('run', type=Path)
    ap.add_argument('--fixture', default='', help='Describe every deliberate setup change')
    ap.add_argument('--out', type=Path)
    ap.add_argument('--measure-after', help='Ignore setup deltas through this snapshot when detecting earned EXP')
    a = ap.parse_args()
    run = a.run.resolve()
    rows = []
    for p in (run/'dumps').glob('*.wram.bin'):
        tag = p.name.removesuffix('.wram.bin')
        info = json.loads((p.parent/(tag+'.info.json')).read_text())
        ram = p.read_bytes()
        word = lambda off: int.from_bytes(ram[off:off+2], 'little')
        entities = []
        for off in range(0x540, 0xD40, 0x40):
            if not ram[off]:
                continue
            entities.append(dict(address=f'{off:04X}', flags=f'{word(off):04X}',
                                 x=word(off+2), y=word(off+4),
                                 type=ram[off+0x18], mode=ram[off+0x1B],
                                 hp=word(off+0x20), gold=word(off+0x28),
                                 exp=word(off+0x2A)))
        rows.append(dict(tag=tag, frame=info['frame'], room=ram[0x48E],
                         room_flags=f'{ram[0x51]:02X}',
                         bounds=[word(0x492), word(0x494)],
                         player=dict(x=word(0x502), y=word(0x504),
                                     flags=f'{word(0x500):04X}', hp=word(0x520),
                                     mp=word(0x522), strength=word(0x524), defence=word(0x526),
                                     gold=word(0x528), exp=word(0x52A),
                                     level=ram[0x52C], weapon=ram[0x530], buff=ram[0x533],
                                     attack_phase=ram[0x511]), entities=entities))
    rows.sort(key=lambda row: (row['frame'], row['tag']))
    events = []
    for old, new in zip(rows, rows[1:]):
        changes = {}
        for key in ('hp', 'mp', 'strength', 'defence', 'gold', 'exp', 'level', 'weapon'):
            before, after = old['player'][key], new['player'][key]
            if before != after:
                changes[key] = dict(before=before, after=after, delta=after-before)
        if old['room'] != new['room']:
            changes['room'] = [old['room'], new['room']]
        old_entities = {e['address']: e for e in old['entities']}
        for entity in new['entities']:
            previous = old_entities.get(entity['address'])
            if (old['room'] == new['room'] and previous and
                previous['type'] == entity['type'] and previous['mode'] == entity['mode'] and
                previous['hp'] != entity['hp']):
                changes['entity_'+entity['address']+'_hp'] = [previous['hp'], entity['hp']]
        if old['room'] == new['room']:
            remaining = {entity['address'] for entity in new['entities']}
            removed = sorted(set(old_entities)-remaining)
            if removed:
                changes['entity_slots_removed'] = removed
        if changes:
            events.append(dict(from_tag=old['tag'], to_tag=new['tag'],
                               frame=new['frame'], changes=changes))
    threshold = -1
    if a.measure_after:
        matching = [row for row in rows if row['tag'] == a.measure_after]
        if not matching:
            raise SystemExit('Measurement snapshot not found: '+a.measure_after)
        threshold = matching[0]['frame']
    result = dict(run=str(run), fixture=a.fixture or None, snapshots=len(rows),
                  measure_after=a.measure_after,
                  exp_increase_observed=any(e['frame'] > threshold and e['changes'].get('exp', {}).get('delta', 0)>0 for e in events),
                  events=events, rows=rows)
    if a.out:
        a.out.write_text(json.dumps(result, indent=2))
    print(json.dumps({k:v for k,v in result.items() if k != 'rows'}, indent=2))


if __name__ == '__main__':
    main()
