# Current development checkpoint — October 17, 2026

Batch 14 is selected. Read [batch 14](../NATIVE-BATCH14-2026-10-17.md) and the
parent HANDOFF-NATIVE-BATCH14-2026-10-17.md before continuing.

`build/LagoonSNESRecomp.exe`, also frozen as `build/lagoon-native-batch14.exe`.
Executable SHA256: `9b63dc19d3a9d4f92e269a14b28d8375758e3543d7bfd00205151c94b1a4787a`.
Manifest SHA256: `8e77084f01c937896a4aeaef1b91aea464acd7c724325dcb5065f93f5deb2027`.
**348 AOT / 123 declared LLE / 234 roots**; all 346 earlier native modes retained.

## Native recovery and safety

Original HP/MP recovery continuations `01D4A2:M1X0` and `01D4CC:M1X0` now execute
across the game's cooperative waits. Four source-validated descriptors cover
item/revival and actual event/boss caller frames. Single-use provenance checks
original stack bytes, owner return, SP, M/X/E/DB/D, source identity and timeline
generation. Unsupported contexts retain LLE. Successful rollback restore clears
ephemeral authority even when state generation is intentionally preserved.
E1 stack wrapping is unsupported and refused. No netplay qualification is claimed.

Across 26 matched combat/primary replays, 13 execute the new continuations:
**19,892 fewer interpreted instruction executions, including 19,867 nonreturn instructions**.
PC/mode keys are deduplicated within each fixture; these are not 19,892 globally
unique addresses or a measured host speedup. The original guardian nonlocal
92A0 death leaf remains native, while parent `$01:9266` and broader orchestration retain
fallback. All 25 prior timing/audio exact deferrals, camera seams and the `0080E6`
opening interpreter floor remain. No arbitrary jump/fallthrough dispatch was enabled.

## Core menus and widescreen

Menu speed is built in; the old Faster menus flag/plugin is a compatibility alias.
Audited Items/Magic/Equipment openings take 40/38/39 fields, previously 148/146/147.
Room 01 closes take 85 instead of 199; Atland closes 92 instead of 226. Remaining
world-loader work still leaves 56/63 blank fields respectively. Guarded native
resource decoding commits CPU/WRAM only after complete source/frame validation;
unknown contexts execute the original native 9143 body. Original upload/DMA stays.
75 pinned streams pass 300 independent original-instruction oracle cases.

Equipment extends original frame/chain/divider pixels in room 01, Atland and the
cave. Three-area verification covers 1,491 Equipment fields, 3,305 total presentations
and 14 checkpoints, retaining the original title/body/cursor. Existing modal
status/fade/reset checks preserve 2,112 native centers and 13 state/clock samples.
Unsupported layouts/resources remain guarded. The live custom renderer retains
its stable-Atland scope; no new locked 60 FPS or full retexture claim.

## Qualification and reproduction

**99 strict matched-host pairs / 151,417 ordered presentations / 2,657 checkpoints**
pass, including framebuffer, WRAM, VRAM, OAM, palette memory, SRAM, CPU registers
and guest clocks/beam. The 346 control and 348 candidate share the updated host,
SDK and core menu policy. This proves native parity under that policy; historical
slow-menu clocks intentionally differ. Three separate menu pairs add 5,367 ordered
presentations and 18 complete checkpoints. Room 01 native parity retains its
explicit matched narrow configuration; widescreen has separate source proof.
An independent audit rechecked all 21,256 raw checkpoint-component pairs and the
ordered presentation files rather than trusting report booleans alone.

All 79 canonical stock-ROM captures were collected and SDK-ingested under the
actual final 348 executable/program identity: 12,714 unique raw records, zero loss
counters and no warnings. Totals are 779,465,595 interpreted instructions and
2,617,665,034 interpreted guest cycles. Do not credit differences from older
selected-build totals solely to native promotion: core menu/host policy and
metadata also changed. The matched 26 proof above isolates native recovery work.
These observations do not automatically promote code.

Public source/roots/selectors reproduce the selected program. The upstream pin
remains `a6be9756285f26ccc74539c811d4d23d57d8c33d`; the 54-path cumulative framework
patch was applied to a fresh pinned clone and all 54 source texts matched the live
SDK after line-ending normalization. Reconstructed guarded-HLE 3 and continuation
matcher 1 tests pass. The real bridge harness passes 198 checks, including rollback
invalidation with unchanged generation. Preserve the deliberately dirty local
SDK; do not reset it or stage its gitlink. Seven protected owner files are unchanged.

## Remaining work

1. Broader death/revival and boss orchestration, promoting only source-proven
   stack/width contracts with actual native execution and exact matched replays.
2. Remaining menu/transition loader work, other layout/actor ownership checks,
   custom-renderer scope and normally paced frame-time measurements.
3. Natural campaign progression, records 2A/3F entry classification, full credits,
   optional soundtrack/audio and controller/GPU hardware qualification. The
   optional in-game map should follow source-backed room/door evidence.

Audio diagnostic captures remain optional; accelerated PCM output-ring overflow
can differ even for an identical executable. No full audio or hardware parity is
claimed. Smooth stays removed, Enhanced optional, state format 3/raster journals
mandatory and retexture shelved. Publish substantial ROM-free source batches.
