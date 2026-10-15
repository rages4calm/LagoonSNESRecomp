"""Reject generated native bodies that would ignore optional camera operands."""
import json
from pathlib import Path
import sys


# Operand byte addresses in the original interpreter-only camera/initializer.
CAMERA_OPERANDS = (0x018408, 0x018432, 0x01845F, 0x018489, 0x0187F6)


def main():
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'src/gen/program_manifest.json'
    manifest = json.loads(path.read_text(encoding='utf-8'))
    conflicts = []
    for key, node in manifest['nodes'].items():
        if node['disposition'] != 'aot_eligible':
            continue
        low, high = node['min_pc24'], node['max_pc24']
        # Conservative whole-body bounds include immediate operands, whereas
        # max_pc24 itself names the last instruction's opcode.
        if any(low <= operand <= high + 3 for operand in CAMERA_OPERANDS):
            conflicts.append(key)
    if conflicts:
        raise SystemExit('Camera ROM patches overlap native bodies: ' + ', '.join(conflicts) + '. Keep their containing entries interpreted before building.')
    print('Camera patch safety: no overlapping generated native bodies')


if __name__ == '__main__':
    main()
