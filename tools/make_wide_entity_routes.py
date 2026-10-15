"""Create bounded presentation fixtures in a private directory.

Use --natural for walking/streaming checks. Default position fixtures are actor
checks only: abrupt jumps can leave missing native scenery. Both initialize rooms;
the cave prefix includes the existing guard bypass, with no story flag changes.
Enable camera_center for paired normal/wide runs. No actor, graphics, HP, EXP,
gold or equipment values are changed by these generated extensions.
"""
import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--natural', action='store_true', help='Walk normally after the room-entry prefix')
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    args.out.mkdir(parents=True, exist_ok=True)
    reset = 'reset\nwait 30\ndump after-reset\nquit\n'
    town = (source/'script_native_gameplay.txt').read_text().split('dump town', 1)[0]
    cave = (source/'script_progression_batch3_gold_fixture.txt').read_text().split('dump cave-entered', 1)[0]
    routes = {
        'town': town + 'dump town\npoke 0502 30021001\nwait 12\ndump npc-right\n'
                'poke 0502 60031001\nwait 12\ndump npc-left\nwait 12\ndump npc-left-settled\n',
        'cave': cave + 'dump cave-entered\npoke 0502 3005b005\nwait 6\ndump bats-06\n'
                'wait 6\ndump bats-12\nwait 6\ndump bats-18\nwait 6\ndump bats-24\n',
    }
    if args.natural:
        routes = {
            'town': town + 'dump town\npress down 132\npress left 100\npress right 198\ndump natural-npc-margins\nwait 12\ndump natural-settled\n',
            'cave': cave + 'dump cave-entered\npress up 80\ndump cave-natural-up\npress left 50\ndump cave-natural-left-margin\npress left 40\ndump cave-natural-right-margin\nwait 12\ndump cave-natural-settled\n',
        }
    for name, route in routes.items():
        kind = 'natural' if args.natural else 'position'
        target = args.out/f'script-wide-{name}-{kind}.txt'
        if target.exists():
            raise SystemExit(f'Refusing to replace {target}')
        target.write_text('# BOUNDED PRESENTATION ROUTE: see tools/make_wide_entity_routes.py\n' + route + (reset if not args.natural else 'quit\n'))
        print(target.resolve())


if __name__ == '__main__':
    main()
