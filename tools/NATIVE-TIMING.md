# Native timing qualification

The interpreter remains part of normal execution. Native promotion must preserve
ordered pictures, WRAM, VRAM, OAM, palette memory, CPU registers and guest clocks
on the same isolated input route. A matching screenshot at a few checkpoints is
insufficient: an earlier private candidate differed for only one Ella field.

## Interpreter-owned instruction timing

Precise native roots use one architectural instruction per generated block.
An unresolved terminal indirect jump hands its own PC to the interpreter, which
fetches and executes that instruction. The generated block must not charge its
cycles first. Doing both charged the same jump twice; the first observed example
was `$02:959D`, adding 36 master clocks before the interpreter executed it. The
same defect occurred at `$01:8195` and other indirect jump sites.

`recompiler/v2/emit_function.py` now omits both static and dynamic charges for
these interpreter-owned precise blocks, including the balanced terminal dispatch
form. Earlier native instructions retain their charges. Historical aggregate
blocks retain their previous output during staged promotion. Focused tests in
`tests/test_precise_root_timing.py` exercise both fallbacks and preservation of
ordinary native instruction timing.

## Mode-aware isolation

The diagnostic `SNESRECOMP_LLE_INTERP_TARGET_FILE` accepts one hexadecimal PC or
an exact `PC:M0X0` variant per line. Plain PCs still deny every mode; for example,
`0084E9:M1X1` leaves the existing `0084E9:M1X0` body enabled. Comments begin with
`#`. Malformed tokens fail the diagnostic run instead of silently changing the
requested exclusion. The interpreter bounce gate and generated body-entry guard
share this matcher. `tests/aot_deny_modes_test.c` exercises the production parser.

This distinction matters when testing newly promoted modes at an address that
already has a qualified native mode. Denying the entire address changes the
control build and can produce misleading timing differences.

## Current staged boundary

After the indirect-jump correction, the larger private candidate restored all
861 observed Ella pictures and its checkpoint memory/register state. Small clock
differences and a single transient field during the six-guardian encounter still
remained. Keeping the 20 newly proposed bank `$00` variants interpreted restored
all pictures, full checkpoint components and guest clocks for Ella, three other
previously failing rooms and the stock six-guardian route. The same addresses'
earlier qualified modes remain native.

This is a tested exclusion boundary, not proof that every excluded routine has
the same underlying defect. Their interrupt, display and scheduling interactions
need further instruction-level investigation before promotion. Native combat and
progression work in other banks can advance independently while these routines
retain the interpreter safety net. Final acceptance uses an actual regenerated
program with those exact variants excluded, rather than shipping a diagnostic
deny file or accepting differences as harmless.

## Precise callers entering aggregate callees

A precise native caller can transfer to an earlier native body that still uses
aggregate block timing. The interpreter commits the completed JSR/JSL before
bouncing into that body. A precise generated caller previously advanced the CPU
clock but left the beam behind until the next synchronization inside the callee.
An instruction trace at `$01:8871` showed this directly: CPU clocks matched when
entering `$00:9143`, while the native path's beam was 62 master clocks behind.

`SNESRECOMP_PRECISE_AOT_TRANSFER_ROOTS` stages the corrected boundary for explicit
new PC/mode variants. Their generated native and paired-interpreter transfer expressions commit timing after
guest return-frame writes and bank changes, immediately before entering the
target body. Native tail transfers use the same rule. Previously qualified roots keep
their existing output unless explicitly selected. The generation cache binds
this option so changing the selector cannot reuse stale function bodies.

Combined with the bank `$00` exclusions, the private prototype restored all
4,521 ordered pictures and 100 full checkpoints and clocks in the original
first-boss defeat route, including its seven previously differing post-victory
checkpoints. The final 326-variant program was then regenerated from public
policy and qualified across the broader campaign fixtures before selection.

## Actor-event deferral

Longer Samson, Battler and Ella victory routes isolated one additional exact
variant, `$01:C2CD:M1X0`. Interpreting this variant alone restored all ordered
pictures, checkpoint components and guest clocks on those three routes. It reads
actor event bytes `$053B..053D + X`, calls `$01:C307`, and passes event commands
to the earlier native `$00:8B71` path. This is a tested timing-sensitive boundary;
it is not evidence that the actor-event data calculations themselves are wrong.
The qualified native bodies in other modes and at other addresses remain
independent of this deferral.

## Graphics deferral and accepted program

Graphics helper `02D1C5:M1X0` separately changed observed beam position in
replays. Keeping that exact new mode interpreted restored those checks. Together
with the 20 bank-00 modes and actor-event mode, these 22 deferrals leave 326
native variants, including all 190 previously selected bodies. They are explicit
generation policy, not a runtime diagnostic override.

The selected program passes 95 frozen comparisons: 135,478 ordered pictures
and 1,704 full checkpoints, including clocks and beam position. These are assisted
encounter regressions against the previous selected program. They do not prove
pure-interpreter equivalence, independent-console timing or natural completion.
See [batch 11](../NATIVE-BATCH11-2026-10-17.md).

## Bus-access experiment retained privately

A separate instruction-start bus-access prototype restored the six-guardian
encounter's transient picture while preserving its 110 checkpoints and clocks,
but did not resolve Ella's remaining clock differences. Applied to the smaller
candidate with bank `$00` exclusions, it changed nothing on four additional boss
routes. That experimental bus behavior remains disabled. Its evidence narrows
future investigation; it does not replace strict qualification.


## Batch 12 continuation

The selected program is now 341 variants, retaining all 326 earlier modes.
Fifteen new exact modes use precise instruction/transfer timing. The HP/MP cap
leaf and balanced hit-selector prefix now run natively; the unresolved selector
jump and broader sword/death continuations retain interpreter ownership.
Three new audio modes and two camera entries remain interpreted in addition to
the prior 22 deferrals. Strict comparison now requires SRAM too. Audio event
journals/full APU state are outside this comparator's equality contract.
See [batch 12](../NATIVE-BATCH12-2026-10-17.md) for the 99 accepted comparisons
and the separately qualified intentional room-01 widescreen margin changes.


## Batch 13 death and cooperative recovery

Original92A0 nonlocal guardian escape is native under the existing paired-call
ABI; its parent9266 remains interpreted. Six actual double-PLA escapes preserve
guest PC/SP, memory, clocks and pictures. D481 and recovery entries/prefixes use
source-specific940D normal-return closure, with no generic HUD contract change.
Additional post-wait roots did not gain native execution: current LLE dispatch
bounces at real calls, not arbitrary jumps/fallthroughs. SPC-poll937C similarly
remains inside its interpreted parent and is excluded from native gain claims.
Native recovery after the first yield needs a proven continuation ABI, rather
than more entry declarations. See [batch13](../NATIVE-BATCH13-2026-10-17.md).

The strict comparator additionally checks declared APURAM/DSP-register/PCM
outputs by SHA256, length and run-relative output path. All other diagnostics
retain exact input semantics. Full captures must exist inside their isolated
run. Optional PCM is s16 stereo and must have complete four-byte sample frames.
Current DSPOUT reads retained host-ring samples: accelerated ring overflow can
discard samples differently even for the same executable. Those experiments
fail strict comparison and provide no audio parity acceptance. No audio runtime
behavior changed in this batch.
