# Save-state execution and cold-load checks

F1 loads slot 1; Shift+F1 saves it. F2 through F10 select the other slots with
Shift to save. F11 opens the state browser. Tests use fresh private save roots.

Earlier builds stored WRAM, PPU memory and a dormant emulator CPU, omitting the
actual runtime CPU and frame cursor. Loading that blob could resume unrelated
boot code against an in-progress world, producing flashing and frozen controls.
Saving now includes the live CPU, interpreter bridge continuation, shared runtime
timing/residue, beam cursor/WAI state, PPU ports/latches and both raster register journals. The missing PPU
VMAIN port was separately caught by a cold restart: same-process restores looked
correct, while cold uploads corrupted VRAM. The new extension restores it too.

The existing framework RSGUARD envelope now checks a whole-payload CRC32 and the
versioned Lagoon state identity (`lagoon-native-state-v3`) before any guest mutation. Inside that envelope,
the RTLS guest schema remains version 10. Lagoon's title extension is
LGA1 audio data followed by a versioned LGE2 execution payload (format 3), checked
for exact size, an additional execution checksum, runtime layout and valid beam state before the first
guest byte changes. These are matching-runtime development states. Earlier
development state formats are refused cleanly; start a fresh game and save with the
updated build. Failed loading reports a message and keeps the running session.
There is no checkpoint-specific recovery path in production.

## Cold-load flashing repair

A moving-town state exposed an omitted display continuation: the PPU has two
raster register journals. A warm reload kept compatible journals in memory,
while a cold reload reused a reset-era buffer every other field. CPU, WRAM,
VRAM, OAM, CGRAM and even the complete register-write trace matched, but every
second reconstructed framebuffer was black. Both SDL and Enhanced/OpenGL
presented that same incorrect source frame.

Format 3 includes an explicitly encoded, pointer-free PRJ1 raster blob: both
field baselines, latches, HDMA enables, ordered writes, drop counters, current
journal and completion/arming state. Pending writes after V=225 survive too.
Loading validates bounds, counts, register addresses and keys before mutation;
render pointers and temporary replay cursors are rebuilt. The earlier format 2
is refused rather than loaded with an incomplete display continuation.

The repaired moving-town cold load matches 64 consecutive warm-reload fields
and 62 uninterrupted fields at all seven state/clock files. None is black.
Hidden actual-GPU SDL and Enhanced/OpenGL checks each capture 141 fields;
only the pre-load startup field is black, followed by 140 nonblack fields.
These are consecutive-field checks, not sparse screenshots. Public reproduction
routes `tools/script_state_moving_natural.txt`,
`tools/script_state_moving_reload.txt` and
`tools/script_state_moving_coldload.txt`, plus the raster serializer unit test,
retain this regression. The natural/reload routes produce a private slot-0
state; seed its copy into the cold-load run with `--seed-save-root`. Compare
matching guest frame numbers because the cold process host counter differs.

## Bounded runtime verification

`tools/script_state_roundtrip.txt` reaches Atland with ordinary inputs, saves,
moves, restores and repeats the same inputs. `tools/script_state_coldload.txt`
loads a private copy of that state in a new process and repeats the movement.
All six snapshot files plus full frame/clock metadata match exactly in the
same-process and cold-process cases. This includes VRAM, OAM, CGRAM, framebuffer,
registers and WRAM. Six refusal tests alter guest payload CRC, guard identity, a beam cursor,
a raster entry count (the latter two with recomputed checksums), file length
and an older format-2 state; each
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
