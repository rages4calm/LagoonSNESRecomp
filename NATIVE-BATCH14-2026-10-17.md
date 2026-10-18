# Native recovery loops and core menus, batch 14 — October 17, 2026

The selected program has **348 native variants, 123 declared interpreter variants
and 234 analysis roots**, retaining all 346 prior native modes. The interpreter
safety net, 25 exact timing/audio deferrals, camera exclusions and opening
`0080E6` interpreter floor remain in place.

## Recovery after the original wait

The original HP and MP recovery continuations `01D4A2:M1X0` and `01D4CC:M1X0`
now execute natively after cooperative waits. Earlier extra roots alone did no
additional native work because the interpreter retained the caller frame. This
batch adds an opt-in continuation ABI with four source-validated descriptors
for the actual item/revival and event/boss callers, preserving original frames
and instructions rather than writing HP or synthesizing a completion result.

Single-use provenance checks the complete stack witness, original owner return,
SP, M/X/E/DB/D, source bytes and timeline generation. Alternate-owner rearming
invalidates stale authority. Synchronous waits cannot leave ready tokens, and
rollback restore discards tokens even when state generation is preserved.
E1 stack wrapping is refused. Existing bounce-disable/exclusion controls still
apply. Unsupported contexts stay on the original interpreter path; arbitrary
jump/fallthrough dispatch remains unchanged. These checks do not qualify netplay.

All 26 combat/primary replays match the updated-host 346 control exactly.
**13 fixtures execute the new continuations, removing 19,892 interpreted instruction
executions, including 19,867 nonreturn executions.** The two generated bodies
contain 17 globally unique instruction-PC/mode keys. Execution accounting is
exclusive within each fixture and deduplicated across overlapping root bodies. This is a
sum across replay fixtures, not 19,892 globally unique addresses, a single natural
playthrough, a wall-clock speedup or a native-completion percentage.

## Menu speed is core behavior

Items, Magic and Equipment acceleration no longer requires a launcher toggle.
The old `fast_menus` feature/plugin is retained only as a compatibility alias.
Guarded original-resource decoding and bounded fade steps preserve the original
upload/DMA pipeline. The decoder transaction validates source identity, resource
sizes, complete return frames, CPU mode and private CPU/WRAM results before
commit. Unsupported contexts retain the original generated `$00:9143` body.
The SDK's optional `hle_guard_func` does not manufacture eligible native modes.

Measured transition lengths use actual menu inputs, from the first dimmed field
to full brightness:

| Transition | Historical fields | Current fields |
| --- | ---: | ---: |
| Room 01 Items opening | 148 | 40 |
| Room 01 Magic opening | 146 | 38 |
| Room 01 Equipment opening | 147 | 39 |
| Room 01 submenu closing | 199 | 85 |
| Atland submenu closing | 226 | 92 |

Original loader work still leaves **56 blank fields in room 01 and 63 in Atland**
on closing. Menus are faster, but are not instantaneous. Historical slow-menu
clock parity is intentionally not claimed. Three same-core menu pairs preserve
5,367 ordered presentations and 18 complete state/clock checkpoints.

All 75 pinned streams pass 300 independent original-instruction oracle cases:
three opening streams, 70 observed world BG/OBJ streams and two common close/HUD
resources, each under four poisoned WRAM/CPU-flag seeds. Entire 128 KiB WRAM and
CPU state are checked. Unknown streams retain the original decoder.
See [core menu details](docs/FAST-MENUS.md).

## Widescreen Equipment and modal checks

Equipment extends original frame, chain and divider pixels in room 01, Atland
and the cave. Original title, body, icons and cursor stay intact. Three-area
verification covers **1,491 Equipment fields, 3,305 total presentations and 14
checkpoints**, including independent original-source pixel checks.

Existing status/fade/reset routes retain **2,112 native centers and 13 full
state/clock checkpoints**. Source and bounds guards continue to reject unknown
resources or stale ownership. Remaining Save/scripted layouts and transitions
need their own qualification. The live custom renderer retains its existing
stable-Atland scope. No locked-60-FPS or full-game retexture claim is added.

## Qualification and reproduction

The final 348 candidate and 346 control share the same updated host, SDK, core
menus and widescreen source. **99 strict pairs pass: 151,417 ordered
presentations and 2,657 complete checkpoints**, including framebuffer, WRAM,
VRAM, OAM, palette memory, SRAM, registers and guest clocks/beam position.
Independent review rechecks the ordered files and all **21,256 raw component
pairs**. Room 01 native parity retains the explicit matched narrow configuration;
wide changes have separate source/pixel verification. No mismatch mask is used.

The actual final executable's 79 canonical stock-ROM captures were collected
and SDK-ingested as one identity: 779,465,595 interpreted instructions,
2,617,665,034 interpreted guest cycles, 8,372 PC/mode variants (8,363 addresses),
515 transfer variants and 12,714 unique raw records. Capture IDs are unique,
all four loss counters are zero and warnings are empty. These are observation
totals, not native gain percentages. Historical 346 totals also differ in host
menu policy/metadata; the matched 26-fixture proof isolates recovery work.
Ingestion does not automatically promote code.

A fresh upstream-pinned clone accepts the 54-path cumulative framework patch;
all 54 source texts match the live SDK after line-ending normalization. Actual
reconstructed guarded-HLE tests (3), continuation matcher test (1), and the real
bridge harness (198 checks) pass. The rollback test reproduces the stale-token
defect when invalidation is removed. Public roots/config/selectors reproduce
the program without distributing private captures or ROM-derived generated C.
The isolated public regeneration matches the manifest, dispatch and existing
function bodies byte for byte. Eight additional legacy interpreter aliases
depend on the synced header; the module build digest also reflects generator
and environment identity. The complete generated file set is not byte-identical.

Selected executable SHA256:
`9b63dc19d3a9d4f92e269a14b28d8375758e3543d7bfd00205151c94b1a4787a`.
Program manifest SHA256:
`8e77084f01c937896a4aeaef1b91aea464acd7c724325dcb5065f93f5deb2027`.
Upstream pin: `a6be9756285f26ccc74539c811d4d23d57d8c33d`.
Framework patch SHA256:
`8e8e05e9b4d7d7a589ec0e740185b344fb98bf7bb553d3c154801bcdf5bf7c9e`.

The normal test entry is `build/LagoonSNESRecomp.exe`; the frozen batch copy is
`build/lagoon-native-batch14.exe`. Seven protected owner files remain unchanged.
ROMs, generated C, graphics/audio, saves, states and raw replay evidence remain
private. Source publication is one substantial batch, not a per-feature push.

Broader death/boss orchestration, remaining menu-close loader work, records
2A/3F entry classification, natural campaign/credits, soundtrack/audio and wider
hardware/frame-time qualification remain open. The interpreter stays until each
replacement has source, actual execution and strict replay evidence.
