#!/usr/bin/env python3
"""Insert `dump <tag>` lines into a pad script at target frames.

Usage:
  python tools/add_dumps.py tools/script_intro.txt 1600=sc-1600 3000=sc-3000 6000=sc-6000 9000=sc-9000

Recomputes the frame each command lands on (wait N = N frames, press b N = N held
+ 1 idle), so the dumps land where they were asked for instead of wherever a
hand-count put them, and prints the schedule it ended up with. Dumps cost no
frames, so the route's present stream is unchanged — which is itself worth
checking afterwards.
"""
import sys


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    path = argv[0]
    want = []
    for spec in argv[1:]:
        frame, tag = spec.split('=', 1)
        want.append((int(frame), tag))
    lines = open(path, encoding='utf-8').read().splitlines()

    def cost(words):
        if words[0] == 'wait':
            return int(words[1])
        if words[0] == 'press':
            return int(words[2]) + 1 if len(words) > 2 else 7
        return 0

    out, frame, pending, schedule = [], 0, list(want), []
    for line in lines:
        words = line.split()
        while pending and words and frame >= pending[0][0]:
            tgt, tag = pending.pop(0)
            out.append(f'dump {tag}')
            schedule.append((tag, frame))
        out.append(line)
        if words and not line.startswith('#'):
            frame += cost(words)
    for tgt, tag in pending:
        out.append(f'dump {tag}')
        schedule.append((tag, frame))
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
    for tag, f in schedule:
        print(f'  dump {tag:10s} at frame {f}')
    print(f'{path}: {len(schedule)} dump lines inserted, route ends at frame {frame}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
