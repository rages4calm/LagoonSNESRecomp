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

A generated root is not proof of live execution. In particular, the bank-table
`$00:80E6` dispatcher still retains interpreter ownership; compiling a target
alone does not authorize an unwitnessed native handoff. Source/mode/stack/owner
and state-generation guards are required for any future bridge.

## Practical tests

Run affected menus, fights, doors and transitions using existing assisted routes.
Compare complete checkpoints and clocks as well as ordered presentation streams.
Separate diagnostic paths from guest differences; keep audio/event/scheduler
limits explicit. Do not repeat every past campaign audit for a focused change.

See [analysis inputs](../recomp/README.md),
[campaign test guard](CAMPAIGN-WARPS.md) and [current scope](../docs/DEVELOPMENT.md).
