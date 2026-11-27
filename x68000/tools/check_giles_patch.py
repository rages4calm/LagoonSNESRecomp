"""Check the pinned English Giles event patch without running the game."""

import argparse
import hashlib
import json
from pathlib import Path

from lagoon_qol import GILES_EVENT_SHA256, giles_rescue_event, patch_giles_rescue


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--system", type=Path, required=True,
                        help="Clean, translated System DIM (never written).")
    parser.add_argument("--event", type=Path, required=True,
                        help="Clean IVent/GOLD03.BIN from the same translation.")
    parser.add_argument("--program", type=Path, required=True,
                        help="Clean translated LAGOON.X, including HU header.")
    args = parser.parse_args()
    event = args.event.read_bytes()
    patched = giles_rescue_event(event)
    assert hashlib.sha256(event).hexdigest() == GILES_EVENT_SHA256
    assert len(patched) == len(event)
    changed = [i for i, (a, b) in enumerate(zip(event, patched)) if a != b]
    assert changed == list(range(0x45, 0x4A)), changed
    assert patched[0x45:0x4A] == bytes.fromhex("10 012a 0000")
    assert patched[0x12A:] == event[0x12A:]
    assert patched[0x128:0x12A] == bytes.fromhex("14 3f")
    assert bytes.fromhex("0e 01 01") in patched[0x12A:0x1F6]
    # Establish opcode $10 from the actual guest dispatch, rather than
    # confusing it with $20 (which removes an actor).
    program = args.program.read_bytes()[64:]
    table = 0x4DD0
    relative = int.from_bytes(program[table + 0x10 * 2:table + 0x10 * 2 + 2],
                              "big", signed=True)
    handler = table + relative
    assert handler == 0x800, hex(handler)
    assert program[handler:handler + 18] == bytes.fromhex(
        "42 80 10 18 e1 80 10 18 41 f9 00 00 20 2e d1 c0 4e75")
    altered = bytearray(event)
    altered[0] ^= 1
    try:
        giles_rescue_event(altered)
    except ValueError:
        pass
    else:
        raise AssertionError("Unrecognized guest event was accepted")
    image = args.system.read_bytes()
    staged = patch_giles_rescue(image)
    disk_changed = [i for i, (a, b) in enumerate(zip(image, staged)) if a != b]
    assert len(image) == len(staged) and len(disk_changed) == 5
    assert disk_changed == list(range(disk_changed[0], disk_changed[0] + 5))
    assert staged[disk_changed[0]:disk_changed[0] + 5] == patched[0x45:0x4A]
    print(json.dumps({"pass": True, "event_sha256": GILES_EVENT_SHA256,
                      "event_changed_offsets": changed,
                      "disk_changed_offsets": disk_changed,
                      "jump_handler": handler, "input_files_written": False}))


if __name__ == "__main__":
    main()
