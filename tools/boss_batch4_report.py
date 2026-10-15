"""Summarize actual boss snapshots and observed call modes, never seed outcomes."""
import sys
from pathlib import Path
# tools/dis.py must not shadow Python's standard-library dis during argparse imports.
sys.path = [p for p in sys.path if Path(p or '.').resolve() != Path(__file__).resolve().parent]
import argparse
import hashlib
import json


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('run', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    run = args.run.resolve()
    receipt = json.loads((run / 'receipt.json').read_text())
    rows = []
    for file in (run / 'dumps').glob('*.wram.bin'):
        tag = file.name.removesuffix('.wram.bin')
        info = json.loads((file.parent / (tag + '.info.json')).read_text())
        ram = file.read_bytes()
        word = lambda a: int.from_bytes(ram[a:a + 2], 'little')
        entities = []
        for off in range(0x540, 0xd40, 0x40):
            if not ram[off]:
                continue
            entities.append(dict(slot=f'{off:04x}', flags=word(off),
                type=ram[off + 0x18], mode=ram[off + 0x1b],
                x=word(off + 2), y=word(off + 4), hp=word(off + 0x20),
                strength=word(off + 0x24), defence=word(off + 0x26),
                gold=word(off + 0x28), exp=word(off + 0x2a),
                phase=ram[off + 0x2c], animation=ram[off + 0x1c]))
        rows.append(dict(tag=tag, frame=info['frame'], room=ram[0x48e],
            player=dict(x=word(0x502), y=word(0x504), hp=word(0x520),
                strength=word(0x524), defence=word(0x526), gold=word(0x528),
                exp=word(0x52a), level=ram[0x52c], weapon=ram[0x530],
                flags=word(0x500), attack_phase=ram[0x511]), entities=entities))
    rows.sort(key=lambda r: (r['frame'], r['tag']))
    events = []
    for old, new in zip(rows, rows[1:]):
        changes = {}
        for key in ('hp', 'gold', 'exp', 'level', 'strength', 'defence', 'weapon'):
            if old['player'][key] != new['player'][key]:
                changes['player_' + key] = [old['player'][key], new['player'][key]]
        if old['room'] != new['room']:
            changes['room'] = [old['room'], new['room']]
        if old['room'] == new['room']:
            before = {e['slot']: e for e in old['entities'] if e['type'] == 3}
            for ent in new['entities']:
                prev = before.get(ent['slot'])
                if ent['type'] == 3 and prev and ent['mode'] == prev['mode'] and ent['hp'] != prev['hp']:
                    changes['boss_' + ent['slot'] + '_hp'] = [prev['hp'], ent['hp']]
        if changes:
            events.append(dict(frame=new['frame'], tag=new['tag'], changes=changes))
    targets = {0x019226, 0x0192c3, 0x0192f1, 0x019306, 0x01b730,
        0x01b9a6, 0x01b9f4, 0x01ba6b, 0x01b7d4, 0x01b81a,
        0x01ace7, 0x01c3e8, 0x01c402, 0x01c486, 0x01c5da}
    calls = []
    for file in (run / 'coverage').rglob('*.json'):
        coverage = json.loads(file.read_text())
        calls.extend(d for d in coverage.get('discoveries', [])
                     if int(d['target_pc24'], 16) in targets)
    route = Path(receipt.get('frozen_route', receipt['route']))
    if not route.exists():
        route = Path(receipt['route'])
    setup = [line.strip() for line in route.read_text().splitlines()
             if line.strip().startswith(('poke ', 'forcepoke ', 'pokefor '))]
    boss_rows = [row for row in rows if row['room'] == 0x40]
    result = dict(executable_sha256=receipt['exe_sha256'], route_sha256=receipt['route_sha256'],
        run=str(run), enabled_features=receipt.get('enabled_features', []),
        enabled_package_features=receipt.get('enabled_package_features', []),
        setup_writes=setup, snapshots=rows, changes=events,
        observed_calls=calls, first_boss_room_40_observed=bool(boss_rows),
        limitations=['Position/weapon-selector setup is explicit; not a natural campaign completion.',
            'HP, damage, rewards and victory are never written by this report.',
            'Observed entry modes/call counts are not full path qualification.',
            'Boss death/rewards require observed real outcomes; no victory is inferred from initialization.'])
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2))
    print(json.dumps(dict(report=str(args.out), snapshots=len(rows),
                         boss_room_snapshots=len(boss_rows), setup_writes=setup), indent=2))


if __name__ == '__main__':
    main()
