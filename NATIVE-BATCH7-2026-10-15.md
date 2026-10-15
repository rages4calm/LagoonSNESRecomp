# Native boss phase, first-scene widescreen and cold-load repair — batch 7

This is a bounded continuation with the interpreter safety net intact. It does
not establish full-campaign compatibility or a completely native port.

## Native player damage and boss logic

Four additional exercised roots cover actor dispatch (`01A7B5`), boss phase
dispatch (`01ACAA`), the first-boss phase (`01ACE7`) and player-hit handling
(`01B9F4`). There are 38 explicit promoted gameplay roots, 181 eligible AOT
variants and 18 declared LLE variants. Other actor and boss handlers have
explicit interpreted boundaries; broad HP subtraction and death remain LLE.

Four frozen replays preserve all six snapshot components and guest clock at
953 checkpoints, plus every ordered frame/CRC across 15,939 presentations.
Across the four new ranges, interpreted work falls by 343,243 instructions,
representing 1,330,591 guest cycles. That measures interpreted workload on these
captures, not wall-clock speedup or campaign completion.

The initial trial compiled ACE7 but did not execute it natively: the interpreted
tail jump never entered that body. The accepted build adds the actual paired
dispatch path and verifies that the first phase's interpreted cost falls to
zero. No exact return width is guessed for shared handlers. The original guest
call frames and nonlocal HP escape remain intact. See
[the native proof and remaining boundaries](tools/DAMAGE-BATCH7.md).

## Widescreen transition coverage

The first Atland world previously appeared at field 1603 while its widescreen
margins waited until 1647. The loader kept `$100E` set through the visible fade.
The new path observes the complete real 14,272-byte background upload, binds it
to the room/resource/timeline and verifies completed PPU graphics and native
maps before allowing early expansion. A DMA start alone is insufficient.

Both original and wide world now become visible at field 1603. All 73 opening
checkpoints preserve native center, guest data and clocks; all 4,198,656 new
margin pixel observations match independently decoded source colors and fade.
The Atland lettering and unsupported intro artwork retain their original layout.
Some cave fades need separate ownership proof for animated torch characters;
those safely retain fallback. See [widescreen evidence](docs/WIDESCREEN.md).

## Flashing and save states

A fresh moving-town state exposed an omitted PPU continuation. After a cold
load, one of the two raster register journals still contained reset-era data,
so the renderer alternated normal scenery and a fully black frame. Guest RAM,
CPU and even PPU write traces could match while this display state was wrong.
Both SDL and Enhanced/OpenGL showed the same bad source frames.

State format 3 now saves both explicitly encoded, pointer-free journals,
including pending post-vblank writes. It validates their layout, counts,
registers and keys before mutation and reconstructs temporary replay state.
Cold reload matches 64 consecutive warm fields and 62 uninterrupted fields at
all seven checked state/clock files. Hidden actual-GPU SDL and EPX captures
show 140 nonblack fields after their one pre-load startup field. The old
alternation is absent in those checks.

General same-process/cold-process state replays also match all seven files.
Six corruption/incomplete-format cases refuse without changing the session,
including a malformed raster count with recomputed outer/inner checksums.
Earlier development state formats are refused; start fresh and save with this
build. The old test checkpoint is not a production recovery requirement.
See [state format, tests and reproduction](docs/SAVE-STATES.md).

## Frame timing and graphics

Reserved-display baseline runs reproduced roughly 60 FPS rather than the prior
46 FPS whole-run average. SDL3 now uses a precise final deadline wait to reduce
scheduler jitter; unnecessary modal scans during sprite uploads are avoided.
Steady-state timing, the final fullscreen measurement and its limits are
recorded in [performance evidence](docs/PERFORMANCE.md). GPU/backend changes
are not used to hide invalid source frames or change guest time.

A separate game-state-driven Lagoon renderer remains a possible future path
for broader scene/HUD rendering. The current SDL backend uses Direct3D 11 on
this machine. Merely replacing the framebuffer uploader with D3D12 does not
establish broader widescreen/native gameplay coverage.

## Qualified source identity

Executable SHA256:
`03fcc1f6eb0fef35c9a3427bf36d1c6aa36adf8faed9103f465d0f670f35b2c2`

Manifest SHA256:
`3d7a1863a4509a5a8bed9e5523e1cd4567705231c01de751e8d1c04f2cd780e2`

The public framework gitlink remains pinned to upstream
`a6be9756285f26ccc74539c811d4d23d57d8c33d`, checked again for this batch. The
cumulative patch reproduces 38 changed framework files, including the raster
serializer regression. Focused timing, JSL suspension, WRAM filters and raster
state tests pass. ROM, generated C, soundtrack files, saves and raw captures
remain private; only requested curated gameplay screenshots accompany source.
