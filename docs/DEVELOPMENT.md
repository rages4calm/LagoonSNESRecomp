# Development

This is a mixed native/interpreter port of the clean USA game. Qualified 65816
routines are compiled to C; unqualified modes, variable scripts and special
hardware paths retain the original-opcode interpreter. Do not replace a fallback
with an empty function, guessed event result or synthetic return.

## Current scope

Scenery contracts cover all 75 authored room descriptors. Native work includes
player damage, death/revival, recovery/reward loops, boss states, Bird/endgame
services, sprite staging, resource/tile queues, numeric UI helpers and original
display fades and queued graphics transfers. The HUD-number dispatcher has a
separate, source-guarded handoff
using its actual cross-bank JSL return frame. Counts of
compiled functions are analysis counts, not completion percentages. Generated
code alone does not prove that a runtime dispatcher reaches a compiled entry.

The assisted campaign work uses original loaders, doors and event/boss scripts,
with position/stat assistance to reach affected cases quickly. It is distinct
from an uninterrupted playthrough. That playtest belongs to the owner and friends.
Earlier checks are not a reason to rerun the entire campaign for every change.

Remaining work includes unsupported special/multipart forms, dynamic graphics,
magic effects and display layouts; additional native variable-script services;
frame-time spikes; and bugs reported during real play. Planned QoL work after
native/rendering stabilization includes permanent escort trail following at
Nasir's pace, an explored map with position, doors, floor connections and
discovered treasure, a small objective journal and optional guidance to known
destinations. These are not implemented. A possible 15–20% zoom-out remains a
separate later consideration.
[Mike's RPG Center maps](https://mikesrpgcenter.com/lagoon/index.html#maps) and the
[walkthrough](https://mikesrpgcenter.com/lagoon/walkthrough.html) are route references,
not proof of runtime state or permission to redistribute their maps.

## Renderer

The custom scene renderer reconstructs supported world backgrounds from actual
resident resources. It verifies the original center picture before replacing a
scene and falls back for unknown room/resource/palette/generation combinations.
It does not stretch a screenshot to fill the margins or invent content past
finite world edges. [Room catalog](RENDERER-COVERAGE.csv).

`tools/entities.py` reads 0x40-byte actor records starting at WRAM `$0500`.
These records are observations of the running guest, rather than host-owned AI.

Additional ordinary actors use finite original sprite tables, exact current
records/frames and resource ownership. They do not predict enemy AI or advance
animation. Selected enemy projectiles use their actual queued records and finite
original frames, within the original projectile lifetime and culling bounds.
They share the original depth order with ordinary actors, including the original
projectile-first ordering when depth keys tie. Supported player-magic packets use
their own verified graphics uploads; fifteen OBJ spell slots have qualified
resource layouts. The split fullscreen BG effect for slot 3 has a separate
contract: preserve its finite original image and extend only the verified
background canvas. Live light/dark phase checks preserve the original center
pixels. The original palette pulses, spell lifetime and culling bounds remain.

Selected multipart boss forms can reuse their actual queued OAM packets when
every cell has resident, verified graphics. The decoder covers thirteen authored
room/kind pairs, including the first two composite boss forms. Whole multipart
groups may use up to 128 cells; ordinary actor groups retain their 64-cell bound.
Focused room 40, 41 and 42 tests confirm live packet recognition but do not show
added margin pixels in those finite arenas. Extended boss pixels have separate
synthetic checks. The room 40 queued prop uses a source-proven draw-time shake
offset before the original post-draw update, with the complete native OAM match
still required. Unsupported forms, unowned optional layers and
Nasir's dynamic graphics retain fallback. Current and retained packets are
withdrawn in the same field when an upload replaces resources they own.
Queued OAM ownership, priority, clipping, palette, camera
coordinates and resource generation must all match before extending a pose.
Changing a guard requires source evidence and affected-route checks.

Title/intro/portrait UI can retain original geometry. Qualified dialogue and
Start views retain their surrounding world; audited Items/Magic/Equipment
presentation and transitions use original assets. Scene transitions need valid
resource provenance, not just the previous room's pixels retained indefinitely.

## Qualification

Keep source, architectural mode, actual guest return-stack and generation guards.
Native promotion is checked against original opcodes with complete memory,
registers, stack and CPU/master/coprocessor clocks, including internal deadlines.
Use matched inputs and controls for affected gameplay, ordered presentations and
complete checkpoints. A matching screenshot alone does not establish correctness.
Timing, audio, scheduler or dead-stack differences must remain explicit.

See [native timing contracts](../tools/NATIVE-TIMING.md),
[analysis policy](../recomp/README.md) and
[bounded campaign warps](../tools/CAMPAIGN-WARPS.md). The cheat hook requires an
isolated bounded offline run with dummy audio, no run-ahead and an actual original
door handoff. Never relax those production guards to obtain a test result.

Frame-time measurements must distinguish normal guest work, rendering,
per-frame diagnostic file I/O, frame limiting and audio high-water waits.
Dummy-audio/hidden-window results do not certify real-device audio or scanout.
Fresh room-loader tests and loaded-state tests differ: loaded states do not
serialize the custom renderer's resource-ownership witnesses. Re-enter through
an authored door before claiming a source-positive far-actor pacing sample.
There is no full-game locked-60-FPS claim.

## Build and contribution

[Build instructions](BUILDING.md). Keep the framework patch and dependency licenses.
Apply/check the cumulative patch on clean checkouts; preserve intentional local
framework changes in active workspaces. Never reset the SDK or accidentally
commit a dirty submodule pointer. ROMs, generated C, decomp exports, states,
extracted assets, audio and raw validation stay private.

Keep changes focused and report what actually ran. Source tests, synthetic
fixtures, assisted routes, installed-artifact checks and natural playthroughs
are different evidence. Commit/push substantial verified batches. Development
handoffs and per-run receipts stay local; Git history preserves old reports.
Per-symbol notes naming retired batch reports refer to that history, including
the [pre-consolidation source](https://github.com/rages4calm/LagoonSNESRecomp/tree/b04a6300d8da11d5d16d6676b2e8b556ce548aee).
