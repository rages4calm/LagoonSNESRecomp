# Save-state execution and cold-load checks

F1 loads slot 1; Shift+F1 saves it. F2 through F10 select the other slots with
Shift to save. F11 opens the state browser. Tests use fresh private save roots.

Earlier builds stored WRAM, PPU memory and a dormant emulator CPU, omitting the
actual runtime CPU and frame cursor. Loading that blob could resume unrelated
boot code against an in-progress world, producing flashing and frozen controls.
Saving now includes the live CPU, interpreter bridge continuation, shared runtime
timing/residue, beam cursor/WAI state and PPU ports/latches. The missing PPU
VMAIN port was separately caught by a cold restart: same-process restores looked
correct, while cold uploads corrupted VRAM. The new extension restores it too.

The existing framework RSGUARD envelope now checks a whole-payload CRC32 and the
versioned Lagoon state identity before any guest mutation. Inside that envelope,
the RTLS guest schema remains version 10. Lagoon's title extension is
LGA1 audio data followed by a versioned LGE2 execution payload (format 2), checked
for exact size, an additional execution checksum, runtime layout and valid beam state before the first
guest byte changes. These are matching-runtime development states. Older
incomplete test states are refused cleanly; start a fresh game and save with the
updated build. Failed loading reports a message and keeps the running session.
There is no checkpoint-specific recovery path in production.

## Bounded runtime verification

`tools/script_state_roundtrip.txt` reaches Atland with ordinary inputs, saves,
moves, restores and repeats the same inputs. `tools/script_state_coldload.txt`
loads a private copy of that state in a new process and repeats the movement.
All six snapshot files plus full frame/clock metadata match exactly in the
same-process and cold-process cases. This includes VRAM, OAM, CGRAM, framebuffer,
registers and WRAM. Five refusal tests alter guest payload CRC, guard identity, a beam cursor
(with recomputed checksums), file length and an older incomplete state; each
refusal matches an equal-frame no-load
control at all seven files, with no guest mutation or console reset.

Reproduce with your clean ROM, matching executable/manifest and private INI:

```
python -P tools/qualify_native.py --exe <exe> --manifest <manifest> --route tools/script_state_roundtrip.txt --features camera_center --config <private.ini> --out <new-roundtrip-dir>
python -P tools/qualify_native.py --exe <exe> --manifest <manifest> --route tools/script_state_coldload.txt --features camera_center --config <same-private.ini> --seed-save-root <new-roundtrip-dir>/saves --out <new-cold-dir>
python -P tools/verify_state_roundtrip.py --saved-run <new-roundtrip-dir> --loaded-run <new-roundtrip-dir> --loaded-tag second-pass --out <private-same-process-report.json>
python -P tools/verify_state_roundtrip.py --saved-run <new-roundtrip-dir> --loaded-run <new-cold-dir> --out <private-cold-report.json>
```

The harness copies seed files and records their hashes. Original inputs are read
only. Save files and extracted snapshots stay private, outside public source.
The general `savestate N`/`loadstate N` script commands use zero-based slots 0..9
and the same save/load path as the keyboard.

These checks establish bounded idle-town continuation, not every campaign
checkpoint. Exact serialization of optional mid-hit recovery/jump-hold timers,
rollback, all devices and other runtime builds still need separate qualification.
