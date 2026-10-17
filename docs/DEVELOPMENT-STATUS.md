# Current development checkpoint — October 17, 2026

Batch 13 is selected. Read [batch 13](../NATIVE-BATCH13-2026-10-17.md) and the
parent HANDOFF-NATIVE-BATCH13-2026-10-17.md before continuing.

`build/LagoonSNESRecomp.exe`, also frozen as `build/lagoon-native-batch13.exe`.
Executable SHA256: `16a1bf96b3b2d2a23ecb8e11cb9eab287e4b74d22847fa30986ee105db4d04d5`.
Manifest SHA256: `27a7dece0b2583433f4495126d5d9c1735e3866790774cf8c5ec496c07366d2a`.
**346 AOT / 123 declared LLE / 232 roots**; all 341 earlier native modes retained.

## Gameplay and rendering

Original death leaf `$01:92A0` now handles the six real guardian double-PLA escapes
natively, preserving PC/SP and all observed state/clocks. Parent `$01:9266` stays
interpreted. Native additions also cover D481 revival and D468/D4AB/D4B1 recovery
entries/prefixes, with a source-specific 940D normal-return fact. Generic HUD
dispatch and broader death/revival orchestration retain fallback. Full recovery
loops still resume interpreted after their cooperative wait; declaration-only
SPC-poll/post-wait experiments are excluded.

Room 01 status and destination fade stay wide through exact original torch DMA
ownership. The prior menu-font diagnosis was incomplete: animation invalidated
the old witness. The final two routes preserve 2,112 native centers and 13 full
checkpoints/clocks, with independent margin decoding, 79 added wide fields and
clean generation reset. Unknown resources and other menu layouts remain guarded.
The custom renderer still covers stable Atland. No new locked-60-FPS claim.

## Qualification

99 strict pairs / 151,417 pictures / 2,657 checkpoints, including SRAM, registers
and guest clocks/beam. Room 01 native parity retains its matched narrow input.
Wide changes are separately source-qualified. 48 project Python tests, seven
helper/real-PPU tests and the mandatory camera build guard pass. Owner save and
settings hashes are unchanged. The actual public regen script reproduces the
manifest and bank/dispatch C; raw selector whitespace can change only module
build-lineage digest. Selection preserves the exact tested executable.

The 79 canonical stock-ROM captures were collected and ingested by the SDK,
with no lost records, reused capture IDs or warnings. They execute 168 fewer
interpreted instructions and 654 fewer interpreted guest cycles than the matched
341-variant control. The separate combat/primary suite measures death/revival
prefix execution; the full recovery loops remain interpreted after yielding.

Optional APURAM/DSP/PCM captures now compare their actual bytes and bound paths.
Accelerated PCM capture is not deterministic when the host output ring overflows;
an identical-executable control confirms that limitation. No audio parity or
playback fix is claimed. Private rejected evidence remains outside accepted QA.

Assisted fixtures disclose position/item/player-stat/flag setup and exercise
original outcomes. Natural campaign, complete credits, independent-console
timing, full audio and broader hardware remain unqualified. Records 2A/3F have
no ordinary incoming doors; their precise entry classification remains open.
Use [map references](MAP-REFERENCES.md) to plan routes and landmarks, then bind
engine IDs and extents to ROM/runtime evidence.

## Next work

1. Native death/revival and boss orchestration with proven stack/width exits.
   For native loops after yields, prove an explicit continuation ABI carrying
   owner entryS/frame kind across the original scheduler; extra roots alone do
   not execute because LLE bounces only at actual JSR/JSL call edges. Preserve
   fallback instead of relaxing arbitrary jump/fallthrough dispatch.
2. More menu/layout/transition and actor ownership checks, custom renderer scope
   and isolated normally paced frame-time measurements.
3. Natural progression,2A/3F classification,credits,controllers/GPU hardware.
   Optional in-game map follows source-backed room/door evidence.

All 25 old timing/audio exact deferrals, camera exclusions and 0080E6 opening
floor remain interpreted. SDK upstream/public pin a6be9756285f26ccc74539c811d4d23d57d8c33d
was verified current; cumulative 47-path patch unchanged. Preserve deliberately
dirty local SDK 273d075a4e7f55e61816e98f91d1ed29f596525c; do not reset/bootstrap or
stage its gitlink. Smooth stays removed, Enhanced optional, stateformat3/raster
journals mandatory, retexture shelved. Publish substantial ROM-free source batches.
