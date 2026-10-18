# Boss animation, menu close and Elf Field, batch 15 — October 17, 2026

Selected batch 15 has **361 AOT variants, 123 declared LLE variants and 244 analysis
roots**, retaining all 348 previous native modes. Final integrated qualification
and cold checks of the selected executable pass. No native completion percentage or natural full-campaign
claim follows from these counts.

## Original boss state and animation work

Thirteen new M1/X0 modes cover original animation counters, staged frame selection,
phase transitions and child sprite states: bank $01: AD69, AE8D, AEA3, AF1E, AF43,
AF7B, AF9B, AFCC, AFE3, B095, B09B, B0C7 and B12B. Their 174 globally unique
instruction-PC/mode keys were checked against the clean ROM. These bodies have
no mode changes, scheduler/audio/device calls or nonlocal frame discards. The
only child call uses already-native AFFA; AF1E's original three-word table and
balanced PHY/PLY prefix are explicitly modeled.

The final integrated build passes **26/26 exact matched-host combat/primary comparisons**.
All 13 new modes execute in these fixtures; 25 of 26 have new work. Exclusive
original-PC/mode accounting removes **186,682 interpreted instruction executions,
including 146,299 nonreturn executions**. Overlapping bodies are deduplicated
within each fixture. These are replay execution totals, not 186,682 distinct
addresses, a single natural playthrough or measured host speedup. Final matched-build coverage binds these counts to the exact candidate/control identities.

The old 25 timing/audio deferrals, camera exclusions and 0080E6 interpreter floor
remain. Existing witnessed HP/MP recovery continuations retain their single-use
source/stack/mode/generation checks. No SDK change or broad jump-dispatch
relaxation is needed for these boss handlers. SPC-poll 937C and its broader
orchestration remain interpreted.

## Original menu-close computation

The earlier built-in menu openings remain 40/38/39 fields for Items/Magic/Equipment.
The new close path performs original map expansion and 1x1 tile queue computation
natively, leaving original uploads, queue-full waits and return seams intact.
At audited room 01 inputs, closing improves from 85 to 60 fields; Atland improves
from 92 to 61. Remaining original work still leaves 31/32 blank fields respectively.
This intentionally changes historical menu clocks; it is qualified separately
from native 361-versus 348 parity under an identical host policy.

Real menu callers, return frames, CPU mode/DB/D, scene flags, source bytes and
resource bounds must validate. Map computation uses a private transactional WRAM
copy. Queue writes happen in place only after all fallible/bounds checks. When
the original queue fills, native work stops before the original 8A97 PHX/PHY/JSL
wait. Runtime tracing witnesses that wait and next-field stack restoration.
Unknown PCs and unsupported contexts refuse without guest or redirect changes.

An independent original-instruction oracle verifies 292 map cases (73 observed
room shapes under four WRAM/flag seeds) and 384 queue cases, including full queue,
sentinels, wrapping, carry and overflow. Entire 128 KiB WRAM and architectural CPU
state match. The public production-hook test adds **635 checks: 15 positive
contexts and 204 readonly refusals**. Five actual captured entries independently
pass; their RAM/CPU snapshots remain private. The previous decoder's 300 oracle
cases remain separate. Reproduce the ROM-free test sources with your clean ROM
through [the core-menu runner](tools/test_core_menus.ps1).

## Elf Field widescreen

Room 08 Elf Field uses guarded original background and actor pixels, including
actual scenery bounds. Selected-build narrow/wide comparisons cover **2,368
presentations and 15 complete checkpoints**, with source/texture checks and
unchanged native centers/guest state. Dialogue, Save/world and close transitions
are audited. The first actual Start panel still takes 35 conservative fallback
fields; room 07 parallax remains unsupported. These cases are not hidden by
relaxing masks or accepting mismatches. See [the widescreen scope](docs/WIDESCREEN.md).

Original-art Equipment and previously qualified room 01 status/fade/reset coverage
remain. The live custom renderer retains its existing stable-Atland scope; the
reference PPU and interpreter remain available. No locked 60 FPS, new graphics API
or full-game retexture claim is made.

## Integrated evidence and source

All 99 final matched-host comparisons pass: 151,417 ordered presentations and
2,657 complete checkpoints, including framebuffer, WRAM, VRAM, OAM, palette,
SRAM, registers and guest clocks/beam. An independent audit rereads every ordered
CSV row and all 21,256 raw component pairs. The selected 361 program and 348 control share
the new menu/widescreen/SDK source. Historical menu timing is intentionally
separate. Room 01 native parity retains its explicit narrow configuration.

Actual final 361 canonical 79 captures were collected and SDK-ingested as one
executable/program identity: 779,259,162 interpreted instructions, 2,616,804,523
interpreted guest cycles, 8,252 PC/mode variants (8,243 addresses),514 transfer
variants and 12,713 unique raw records. All loss counters are zero, IDs are unique
and warnings are empty. These observation totals are not added to the separate
26-fixture execution proof or credited solely to native work: core menu policy
also changed. Ingestion does not automatically promote code. Records 2A/3F remain
entry/classification gaps. Selection is recorded separately from the immutable qualification receipts.

Selected executable SHA256:
`26b96b8f4c2c02e5be908e424df1a44a810baf4573e84afa971b6e964ff366ba`.
Selected program manifest SHA256:
`3d6f473e2c69ba3d26f76c9e8c6b0c15e031845dbec64356d6d678effb86398b`.
Upstream pin remains `a6be9756285f26ccc74539c811d4d23d57d8c33d`; the 54-path
framework patch is unchanged; this batch changes no SDK runtime source. Actual
public regeneration reproduces all 17 generated C files byte-for-byte; only
module build-lineage identity differs. Public configs/roots/selectors reproduce
the program without distributing ROM-derived generated C or private profiles.

Test `build/LagoonSNESRecomp.exe` or the frozen `build/lagoon-native-batch15.exe`.
All seven owner files remain unchanged. Three cold native checks and the direct
Items replay (1,762 presentations/six checkpoints) pass on the selected build.

Keep owner saves/settings, ROMs, extracted art/audio and raw captures private.
Broader death/boss orchestration, remaining menu loader work, room 07/parallax,
records 2A/3F entry classification, natural progression/full credits, optional
audio and hardware/frame-time qualification remain open. Source publication uses
substantial verified batches.
