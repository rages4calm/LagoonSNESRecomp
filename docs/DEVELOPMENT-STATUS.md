# Current development checkpoint — October 17, 2026

Native and widescreen batch 12 is selected. Read [batch 12](../NATIVE-BATCH12-2026-10-17.md)
and the parent HANDOFF-NATIVE-BATCH12-2026-10-17.md before continuing.

## Selected program

`build/LagoonSNESRecomp.exe`, also frozen as `build/lagoon-native-batch12.exe`.
Executable SHA256: `24452e86bcac15964f0668ad6108d6fffd34ebd22cd2ed59735c1c69a3ed6919`.
Manifest SHA256: `5f56797fb866bfaaf6405430f34d02787c0bf7ffe8a760235cc583e5779dd817`.
**341 AOT / 126 declared LLE / 230 analysis roots**, preserving all 326 prior
native modes and adding 15. Interpreter fallback and the opening repair remain.

New native work includes the original HP/MP cap leaf during consumable revival
and boss healing, actor-hit selector setup, actor setup/clamping, collision
direction, event flags and endgame WRAM copying. Broader sword scanning and
death/revival orchestration still use their interpreted continuations. All 22
prior exact timing deferrals, three newly observed audio modes and two camera
entry exclusions stay interpreted. No host HP or victory replacement is used.

## Evidence and limits

99 strict replay pairs pass: 151,417 ordered pictures and 2,657 checkpoints,
including framebuffer, WRAM, VRAM, OAM, palette memory, SRAM, registers, clocks
and beam position. Counts include repeated fixtures. Room 01 is compared in a
fresh matched narrow pair; its historical wide margins intentionally differ.
Separate final-binary widescreen checks preserve all 2,174 native centers and
11 checkpoints, with eight independently decoded original-map samples.

Room 01's exact resources and 512×768 extent are guarded. Status temporarily
uses native fallback when font uploads invalidate world characters, then wide
gameplay resumes. Unsupported areas, actor poses and effects retain fallback.
The custom renderer still covers stable Atland only. No new locked-60-FPS claim.

45 project Python tests, seven helper/real-PPU tests and the camera build guard
pass. The actual public regeneration script reproduces the exact program
manifest and bank/dispatch C. Its module build-lineage digest can differ from
the private CLI's selector whitespace; this does not change the program digest.
The tested executable itself is preserved exactly. Owner saves/settings remain
unchanged. No public binary release, ROM, generated C, audio or raw capture.

All 79 fresh canonical stock-ROM captures were ingested with zero loss/warnings:
779,468,994 interpreted instructions and 2,617,679,683 guest cycles, removing
35,302 interpreted instructions versus matched326 fixtures. Room 01's narrow
host-config exception is recorded. Separate targeted accounting proves
nonreturn work removal for 12 of 15 new modes; two have no observed savings
and one is return-only. No per-variant count implies complete native coverage.

Fixtures explicitly use teleports, bounded player stats, item selection and
prerequisite flags. Original damage, revival-item consumption, healing, boss
deaths, rewards and progress run normally. These are assisted checks against
the prior selected program, not a natural full campaign, independent-console
parity, pure-interpreter timing parity or full APU-state/audio qualification.

## Next priorities

1. Continue source-proven HP subtraction, death/revival and boss orchestration;
   resolve indirect/nonlocal stack contracts before promoting broader routines.
2. Investigate the 22 old timing modes and new audio seams with instruction-level
   traces. Preserve the 00:80E6 opening floor and camera patch exclusions.
3. Broaden widescreen room/menu/transition and actor-source coverage; keep font,
   raster, upload and map-bound guards. Expand custom rendering from measured needs.
4. Natural campaign, script-only cloud entry, record 3F, credits, controller/GPU
   hardware and isolated real-time frame-spike qualification. Optional map later.

Smooth stays removed; Enhanced original pixels remains optional. State format 3
and both raster journals remain required. The full retexture remains shelved.
Public SDK pin remains a6be9756285f26ccc74539c811d4d23d57d8c33d (latest upstream
verified this batch). Its cumulative 47-path patch is unchanged. Preserve local
SDK HEAD 273d075a4e7f55e61816e98f91d1ed29f596525c and dirty/staged source; never
reset/bootstrap or stage its gitlink. Publish substantial verified source batches.
