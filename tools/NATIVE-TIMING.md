# Native timing qualification

The interpreter remains part of normal execution. Native promotion must check
ordered pictures, memory, CPU registers, real saved/dead stack frames and guest
clocks against original opcodes on matched isolated inputs. A few screenshots or
a source compile cannot establish parity. Preserve unexplained differences in
the result; do not turn a failed strict comparison into an unqualified pass.

## Interpreter-owned timing

Precise native roots use instruction-level timing. An unresolved terminal indirect
jump returns its own PC to the interpreter, which fetches and executes it. The
native emitter must not charge those cycles first. Charging both sides previously
added 36 master clocks at an observed indirect jump.

The cumulative framework patch fixes the precise fallback forms in
`recompiler/v2/emit_function.py`. Native instructions retain their charges.
Focused framework tests cover indirect fallback, balanced dispatch and ordinary
instruction timing. Keep previously qualified aggregate bodies unchanged unless
their timing is explicitly included in the new qualification.

## Completed transfers

A precise caller entering an aggregate callee must commit the completed JSR/JSL
after writing the actual guest return frame and changing banks, before invoking
the callee. Otherwise CPU and beam clocks can disagree at the same entry.
Native tail transfers need the same ownership rule.

`SNESRECOMP_PRECISE_AOT_ROOTS` selects instruction timing;
`SNESRECOMP_PRECISE_AOT_TRANSFER_ROOTS` selects the completed-transfer behavior.
The generation cache binds these selectors. The project's
`recomp/precise-timing.txt` and `recomp/transfer-timing.txt` are reviewed policy;
`tools/regen.sh` supplies both. A selector change requires regeneration.

## Exact mode controls

The diagnostic `SNESRECOMP_LLE_INTERP_TARGET_FILE` accepts a six-digit PC or exact
variant, such as `0084E9:M1X1`. A plain PC excludes every architectural mode at
that address. The body-entry guard and interpreter bounce gate share the parser;
malformed tokens fail the run. Denying a whole address when only one new variant
is being checked also changes previously qualified sibling modes.

Production exclusions live in bank cfg files using `force_lle` or
`force_lle_variant`. They must remain in the generated program, rather than
relying on a diagnostic file being present on the player's machine.

## Source and runtime proof

Use complete source bounds through the real RTS/RTL or unresolved handoff.
Original-opcode unit checks should cover flags, hidden accumulator high byte,
width modes, stack positions, actual nested frames, complete memory and timing,
including entry and internal deadlines. A child stop must preserve its real frame;
never simulate the child by inventing a return or outcome.

Constant-bus units are distinct from live hardware/IRQ/scheduler checks. Use a
fresh original-opcode control for the selected services with the same host and
renderer. Validation-only return facts may come only from already proven source;
verify that unrelated native entries, existing facts, exit sets and HLE guards
remain unchanged. Changes to a caller's return width cannot be guessed to make
analysis green.

Constant-bus instruction tests and matching total clocks do not certify live
audio-port event timing. Compare the complete checkpoint APU/DMA/PPU-write logs
as well as the strict comparator's architectural components before accepting a
promotion. A passing door route alone does not qualify a boss route. Keep any
service that changes live event timing on the interpreter until that difference
is resolved; preserve already-qualified sibling modes separately.

The fixed-page transfer leaf `$00:8F8C:M1X0` has a qualified native body. Its
live-device fixture requests the original two queued pages through `$023B`,
using guest-loaded buffers and the real NMI/JSR/DMA/RTS path. That assisted
fixture is distinct from reaching the credits naturally. The frequent M1X0
services `$00:871C`, `$00:8EC1` and `$00:8EE7` also have qualified M1X0 bodies.
Their earlier experimental promotions exposed a periodic APU catchup phase that
counted interpreted work alone. Constant-bus passes were insufficient: promotion
waited for complete same-host boss and door checkpoints, including all audio logs.

Mapped primary-CPU execution now checks periodic APU catchup after actual completed
instruction/beam boundaries. Native and interpreted work use the same absolute
elapsed-master-clock predicate. The cached threshold keeps its original default
of 1024 and environment semantics; unsupported mappings and existing disabled,
shared-cosimulation and legacy paths keep their previous eligibility. Mandatory
port and exit synchronization is preserved. A paired native return commits only
after a normal result consumes its actual two- or three-byte guest frame, with
no pending unwind. It does not invent return cycles or alter the saved stack.

The actual-function guard/refresh tests and a separate real SPC/DSP reference
test check periodic partitioning, scheduled port targets, latch/read events,
serialized device state and nonzero PCM. The reference is an authored SPC
program, not a complete Lagoon soundtrack certification. The trace's sample
column is a process-lifetime output counter; snapshot restore does not rewind
it. Older-host trace differences remain explicit. This distinction does not
permit suppressing differences in the complete matched native/floor comparison.

A generated root is not proof of live execution. The bank-table `$00:80E6`
dispatcher executes its original indirect jump. A separate opt-in hook can lend
the actual three-byte `$00:80E0` JSL frame to the complete `$02:D50B` HUD body,
only for the qualified selector, caller, source bytes, mode, live registers,
stack and state generation. The original `$00:80E4` PLB/RTL cleanup still runs.
Other selectors and contexts retain interpreter ownership. This hook leaves the
existing two-byte absolute/indirect tail policies unchanged.

## Practical tests

Run affected menus, fights, doors and transitions using existing assisted routes.
Compare complete checkpoints and clocks as well as ordered presentation streams.
Separate diagnostic paths from guest differences; keep audio/event/scheduler
limits explicit. Do not repeat every past campaign audit for a focused change.

`tools/test_apu_cadence_guard.py` and `tools/test_apu_cadence_device.py` accept
`--out` for disposable test output and `--compiler` for a C11 compiler. They
extract the actual SDK functions under test and bind source hashes. The first
uses mocked device plumbing for guard and timing invariants; the second links
the actual APU, SPC, DSP and trace engines. Neither replaces affected live routes.

See [analysis inputs](../recomp/README.md),
[campaign test guard](CAMPAIGN-WARPS.md) and [current scope](../docs/DEVELOPMENT.md).
