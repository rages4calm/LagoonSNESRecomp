"""Record actual HP, consumable, boss and reward transitions from batch 5 runs."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--setup', required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    receipt = json.loads((args.run / 'receipt.json').read_text())
    rows = []
    for file in (args.run / 'dumps').glob('*.wram.bin'):
        tag = file.name.removesuffix('.wram.bin')
        ram = file.read_bytes()
        info = json.loads((file.parent / (tag + '.info.json')).read_text())
        word = lambda a: int.from_bytes(ram[a:a+2], 'little')
        rows.append(dict(tag=tag, frame=info['frame'], room=ram[0x48e],
            player_hp=word(0x520), player_flags=word(0x500),
            level=ram[0x52c], strength=word(0x524), defence=word(0x526),
            weapon=ram[0x530], armor=ram[0x531], shield=ram[0x532], item=ram[0x536],
            gold=word(0x528), exp=word(0x52a),
            boss_type=ram[0x558], boss_mode=ram[0x55b], boss_hp=word(0x560),
            boss_flags=word(0x540), story_flags=ram[0x4c0:0x4d0].hex()))
    rows.sort(key=lambda r: (r['frame'], r['tag']))
    tracked = ('player_hp', 'player_flags', 'item', 'gold', 'exp', 'boss_hp', 'boss_flags')
    events = []
    for old, new in zip(rows, rows[1:]):
        changes = {k: [old[k], new[k]] for k in tracked if old[k] != new[k]}
        if changes and old['room'] == new['room']:
            events.append(dict(frame=new['frame'], tag=new['tag'], changes=changes))
    targets = {0x019226, 0x0192a0, 0x0192c3, 0x0192f1, 0x019306, 0x019346,
               0x01ba6b, 0x01ace7, 0x01ddad, 0x01d8da, 0x01d487, 0x01b81a}
    calls = []
    for file in (args.run / 'coverage').rglob('*.json'):
        coverage = json.loads(file.read_text())
        calls.extend(d for d in coverage.get('discoveries', [])
                     if int(d['target_pc24'], 16) in targets)
    route = Path(receipt['frozen_route'])
    result = dict(executable_sha256=receipt['exe_sha256'],
        route_sha256=receipt['route_sha256'], setup=args.setup,
        setup_writes=[line.strip() for line in route.read_text().splitlines()
                      if line.strip().startswith(('poke ', 'forcepoke ', 'pokefor '))],
        enabled_features=receipt['enabled_features'], snapshots=rows,
        actual_transitions=events, observed_entries=calls,
        limitations=['Setup is explicit and does not establish naturally earned progression.',
                     'No victory or campaign completion is inferred from HP or a route name.',
                     'Observed modes do not prove every return, branch or stack contract.'])
    args.out.write_text(json.dumps(result, indent=2))
    print(json.dumps(dict(snapshots=len(rows), entries=len(calls), report=str(args.out))))


if __name__ == '__main__':
    main()
