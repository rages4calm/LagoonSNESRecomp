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

## Graphics deferral and batch 11 acceptance

Graphics helper `02D1C5:M1X0` separately changed observed beam position in
replays. Keeping that exact new mode interpreted restored those checks. Together
with the 20 bank-00 modes and actor-event mode, these 22 deferrals leave 326
native variants, including all 190 previously selected bodies. They are explicit
generation policy, not a runtime diagnostic override.

The batch 11 program passed 95 frozen comparisons: 135,478 ordered pictures
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

Batch 12 selected 341 variants, retaining all 326 earlier modes.
Fifteen new exact modes use precise instruction/transfer timing. The HP/MP cap
leaf and balanced hit-selector prefix now run natively; the unresolved selector
jump and broader sword/death continuations retain interpreter ownership.
Three new audio modes and two camera entries remain interpreted in addition to
the prior 22 deferrals. Strict comparison now requires SRAM too. Audio event
journals/full APU state are outside this comparator's equality contract.
See [batch 12](../NATIVE-BATCH12-2026-10-17.md) for the 99 accepted comparisons
and the separately qualified intentional room-01 widescreen margin changes.


## Batch 13 death and cooperative recovery

Original 92A0 nonlocal guardian escape is native under the existing paired-call
ABI; its parent `$01:9266` remains interpreted. Six actual double-PLA escapes preserve
guest PC/SP, memory, clocks and pictures. D481 and recovery entries/prefixes use
source-specific940D normal-return closure, with no generic HUD contract change.
Additional post-wait roots did not gain native execution: current LLE dispatch
bounces at real calls, not arbitrary jumps/fallthroughs. SPC-poll 937C similarly
remains inside its interpreted parent and is excluded from native gain claims.
Native recovery after the first yield needs a proven continuation ABI, rather
than more entry declarations. See [batch 13](../NATIVE-BATCH13-2026-10-17.md).

The strict comparator additionally checks declared APURAM/DSP-register/PCM
outputs by SHA256, length and run-relative output path. All other diagnostics
retain exact input semantics. Full captures must exist inside their isolated
run. Optional PCM is s16 stereo and must have complete four-byte sample frames.
Current DSPOUT reads retained host-ring samples: accelerated ring overflow can
discard samples differently even for the same executable. Those experiments
fail strict comparison and provide no audio parity acceptance. No audio runtime
behavior changed in this batch.


## Batch 14: source-guarded recovery continuations

The selected 348-variant program retains all 346 prior modes and adds only
`01D4A2:M1X0` and `01D4CC:M1X0`. Declaring these PCs alone previously produced no
native loop execution. The opt-in bridge now witnesses actual original wait
calls, preserves their real guest frames and permits a native continuation only
at a source-proven original RTL. Four title descriptors distinguish item/revival
owners from the actual event/boss owners. No generic jump dispatch is relaxed.

A single-use token binds SP, the complete wait/saved-register/owner frame,
M/X/E/DB/D, source guard and state generation. Reentrant owner alternatives
invalidate old authority; stale serials cannot activate replacement tokens.
Normal/synchronous waits do not leave ready tokens. Loading a rollback residue
clears tokens because rollback can preserve state generation. Refusal leaves
guest state untouched and uses the original interpreter path. E1 descriptors
are rejected until wrapped stack witnesses exist. The continuation path also
honors the existing bounce-disable and exact-mode exclusion gates, and clears
the interpreter write-site context before native dispatch.

Actual matched 346/ 348 comparisons pass 99/99: 151,417 ordered presentations and
2,657 complete checkpoints/clocks, with SRAM required. Both executables share
the updated host/SDK/menu policy. Across the 26 combat/primary pairs, 13 execute
new continuation work: 19,892 interpreted instruction executions disappear, including 19,867
nonreturn instructions. Unique instruction-PC/mode keys are counted within each
fixture and deduplicated across overlapping root bodies. Summing independent
replays is not a count of globally unique PCs or a host-FPS measurement.

A separate disabled-bounce control proves zero native continuation gain. Matcher
and real bridge tests cover wrong mode/stack/source/generation/owner/token,
synchronous and repeated yields, alternate callers and rollback with unchanged
generation. Removing rollback invalidation reproduces the future-token defect.
These are bounded safety checks, not netplay certification.

`hle_guard_func` is a separate opt-in generated entry seam for core menu decoding.
It preserves original native bodies and existing eligible modes. A try-handler
runs after original entry watchdog/deadline checks and before original costs;
false falls through unchanged. Logical inherited entryS/frame/host return are
passed explicitly; true returns use the correct normal/yield host-stack pop.
`--no-hle` disables it; handler/mode metadata and cfg/emitter identity bind its
outputs. Unknown menu contexts retain the original 9143 body. Intentional menu
timing changes are qualified separately, not masked in native comparisons.
See [batch 14](../NATIVE-BATCH14-2026-10-17.md).


## Batch 15: bounded original boss animation/state handlers

The selected 361-mode program retains all 348 previous native modes and adds 13 M1X0
handlers with 174 unique original instruction-PC/mode keys. Original ROM/source
review proves balanced returns, no mode changes or hardware/audio/scheduler
calls, an already-native AFFA child and the exact AF1E three-word phase table.
No SDK change, generic exit widening, arbitrary jump bounce or old timing/audio
exclusion removal is required.

Final matched-host 26 comparisons pass against the 348 control: all 13 new modes execute
and 25 fixtures remove 186,682 interpreted instruction executions, 146,299 excluding
returns. Exclusive PC/mode accounting deduplicates overlapping bodies per replay;
this is not 186,682 distinct addresses or a wall-clock gain. Final matched-host accounting binds the exact 433f control and 26b96 candidate. Core map/tile queue acceleration intentionally
changes old menu clocks, so the final 348 control and 361 candidate share that
same updated host before strict native comparison.

The new close-hook proof checks actual production registration/callbacks,
original return chains, all three submenu owners, CPU mode/DB/D, source/scene
flags and original code hashes. 635 public synthetic checks include 15 positives
and 204 complete CPU/128 KiB-WRAM/redirect readonly refusals. Five private actual
entries additionally pass. Full queue redirects before original 8A97 PHX/PHY/JSL
wait; runtime tracing independently observes that wait and following restoration.
The 676 original-opcode map/queue oracle cases preserve full WRAM and CPU/flags;
previous 300 decoder cases are separate. Unknown callback PCs now explicitly
refuse. See [batch 15](../NATIVE-BATCH15-2026-10-17.md) and [core menu scope](../docs/FAST-MENUS.md).
