"""Check sword preservation and the pinned jump-timer patch; no game inputs are written.

Usage: python -B -P check_combat_patch.py /path/to/original/LAGOON.X
This is a binary regression check, not a runtime combat qualification.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_retrorecomp import PROGRAM_SHA256
from lagoon_qol import enhance_program


def check(program):
    if hashlib.sha256(program).hexdigest() != PROGRAM_SHA256:
        raise ValueError("Expected the original English 0.9b LAGOON.X.")
    original = program[64:]
    modified = enhance_program(program, adventure=True, expedition=True)[64:]
    # Preserve the original input path, sword raise/ready, rise, fall and
    # peak action handlers. Only the timer loaded at jump start changes.
    for start, end in ((0x119EA, 0x11A62), (0x11A66, 0x11B7C)):
        assert original[start:end] == modified[start:end], hex(start)
    assert original[0x11A62:0x11A66] == bytes.fromhex("34bc0012")
    assert modified[0x11A62:0x11A66] == bytes.fromhex("34bc001a")
    dispatch = {
        state: 0x11B16 + struct.unpack_from(">H", original, 0x11B16 + 2 * state)[0]
        for state in (1, 3, 4, 5)
    }
    assert dispatch == {1: 0x11B3A, 3: 0x11B22, 4: 0x11B5A, 5: 0x11B48}
    # Keep physical-contact gating, RNG damage, HP subtraction and the full
    # ordinary/boss death paths identical to the original. Reach is changed
    # upstream in collision detection; it must not rewrite damage rules.
    for start, end in ((0xF04C, 0xF08C), (0xF1AE, 0xF294)):
        assert original[start:end] == modified[start:end], hex(start)
    return {"binary_regression": "passed", "sword_state_handlers": "original",
            "peak_timer_original": 18, "peak_timer_enhanced": 26,
            "physical_damage_and_death": "original",
            "runtime_verified": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("program", type=Path)
    print(json.dumps(check(parser.parse_args().program.read_bytes()), indent=2))
