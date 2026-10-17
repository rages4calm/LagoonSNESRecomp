# Native and widescreen batch 12 — October 17, 2026

The selected program now has **341 native variants**, up from 326, while keeping
the interpreter safety net. All 326 previous modes remain native. Generated
counts describe architectural variants, not campaign completion percentages.
There are 126 declared interpreter variants and 230 analysis roots.

## Native gameplay

The new native work includes the level-dependent HP/MP cap lookup used during
real consumable revival and post-boss healing, the balanced actor-hit selector
prefix, actor setup/clamping, collision-direction helpers, event bits and an
endgame WRAM copy. The broader sword scan and HP/death orchestration retain
their interpreted continuations. No host replacement fabricates HP or victory.

The HP-cap leaf preserves Y with PHY/PLY and reads original level tables.
The selector prefix retains its unresolved indirect jump on the interpreter,
which owns that instruction's timing. Precise instruction and completed-transfer
timing apply to all 15 added exact modes. One added entry is an RTS-only leaf;
native execution accounting separates return-only work from gameplay work.

Across 26 targeted replay pairs, coverage shows nonreturn instruction removal
for 12 of the 15 added modes. Two modes show no observed savings and one is
return-only. The deduplicated instruction sets account for 44,683 fewer
interpreted nonreturn instructions on those replays; overlapping function
ranges are not summed twice. The repeated first-boss fixture is disclosed.

All 22 earlier timing deferrals remain. Three newly observed audio modes also
stay interpreted, and two camera-patched entries remain excluded by the
whole-body build guard. The opening dispatcher at 00:80E6 remains interpreted.
The source policy reproduces the same program manifest through the actual
regeneration script without distributing private coverage or generated C.

## Widescreen

Room $01 now uses guarded extra world scenery with its exact background/object
resources, map definitions and 512×768 bounds. The compositor preserves the
native center and masks outside the actual world. It retains supported outgoing
scenery during entry and restores wide gameplay after status closes. The status
menu still falls back during 34 observed fields because its font upload changes
characters needed by that room's map. Unsupported actor poses remain guarded.

Two final-executable widescreen routes cover **2,174 fields and 11 checkpoints**:
original-loader entry, walking, status open/close and natural return to Atland.
Every native center and checked guest state/clock matches the retained control.
Eight samples match independent ROM-map/VRAM/palette decoding; outside-world
and fallback margins remain black. This adds original artwork, not stretching.
The independent custom scene renderer retains its existing Atland scope.

## Qualification

**99 strict replay pairs pass**, covering **151,417 ordered presentations and
2,657 checkpoints**. Comparisons include framebuffer, WRAM, VRAM, OAM, palette
memory, SRAM, registers, guest master clocks and beam position. SRAM is newly
mandatory. No masks or ignored stack/timing bytes are used. These comparisons
do not certify full APU state or audio parity.

The original room-01 widescreen fixture intentionally changes margin CRCs. That
historical strict comparison remains recorded as a presentation difference.
Native qualification instead uses a fresh matched pair with widescreen disabled;
the new wide margins have their separate source/pixel proof above. No difference
is relabeled an exact match. The 99 comparisons include repeated routes and are
not 99 unique game scenarios.

All 79 canonical stock-ROM captures were collected and actually ingested by the
SDK with zero loss counters or audit warnings. They contain 779,468,994
interpreted instructions and 2,617,679,683 interpreted guest cycles. Against the
matched 326-variant fixture set, that removes 35,302 interpreted instructions.
Room 01's host-presentation config exception is recorded explicitly. These
figures measure the specified replays, not wall-clock speed or game completion.

Tests include **45 project Python tests**, seven helper/real-PPU tests and the
build-time camera overlap guard. Seven protected owner save/settings files remain
byte-identical. The final program was built once and its exact tested executable
was selected without rebuilding afterward.

Executable SHA256: `24452e86bcac15964f0668ad6108d6fffd34ebd22cd2ed59735c1c69a3ed6919`.
Program manifest SHA256: `5f56797fb866bfaaf6405430f34d02787c0bf7ffe8a760235cc583e5779dd817`.

Fixtures disclose teleports, legal/boosted player stats, item selection and
prerequisite flags. Original damage, healing, consumable use, enemy deaths,
rewards and completion outcomes run normally. These are assisted checks, not an
uninterrupted natural campaign, complete credits or hardware certification.
No new locked-60-FPS claim or binary release is made. Launch `Start Lagoon.cmd`.
ROMs, generated C, audio, saves and raw captures remain private.

See [current status](docs/DEVELOPMENT-STATUS.md), [timing](tools/NATIVE-TIMING.md),
[widescreen](docs/WIDESCREEN.md) and [todo](TODO.md).
