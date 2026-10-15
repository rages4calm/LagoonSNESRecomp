"""Reject generated native bodies that would ignore optional camera operands."""
import json
from pathlib import Path
import sys


# Operand byte addresses in the original interpreter-only camera/initializer.
CAMERA_OPERANDS = (0x018408, 0x018432, 0x01845F, 0x018489, 0x0187F6)
CAMERA_RANGES = ((0x01809D,0x0180A0),(0x0180CA,0x0180CD),(0x0183D8,0x0183FB))


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
        # Audited $01:8385 terminates at the single-byte RTS $83D7, directly
        # before the reused dispatcher. Its last opcode is not a 4-byte call.
        # Keep the conservative bound for every other terminal instruction.
        end_exclusive = 0x0183D8 if high == 0x0183D7 else high + 4
        if (any(low <= operand < end_exclusive for operand in CAMERA_OPERANDS) or
            any(low < end and end_exclusive > start for start,end in CAMERA_RANGES)):
            conflicts.append(key)
    if conflicts:
        raise SystemExit('Camera ROM patches overlap native bodies: ' + ', '.join(conflicts) + '. Keep their containing entries interpreted before building.')
    print('Camera patch safety: no overlapping generated native bodies')


if __name__ == '__main__':
    main()
