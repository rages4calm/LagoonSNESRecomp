# Native gameplay batch 11 — October 17, 2026

This batch fixes the larger native candidate's Ella glitch and qualifies a
narrower program with **326 native variants**, up from 190. The interpreter
safety net remains enabled. Counts describe generated variants, not campaign
completion or a claim that every boss routine is native.

## What changed

- An unresolved indirect jump was charged by generated code and then executed
  again by the interpreter. Precise instruction blocks now leave that opcode's
  full charge to its actual execution tier.
- Completed native calls now synchronize guest time before entering either a
  native callee or a paired interpreter callee. This timing policy applies to
  the 136 newly staged native variants; established timing modes are preserved.
- Twenty new bank-00 modes and two individually isolated routines remain
  interpreted. The actor-event/sound path at `01C2CD:M1X0` altered interrupt
  phase in long boss fights. Graphics helper `02D1C5:M1X0` changed observed beam
  position in other fixtures. Exact-mode exclusions preserve native siblings.
- The centered-camera guard incorrectly rejected an unrelated actor helper at
  `01:8158`. Its dispatcher range now covers the actual patched instructions;
  the separate build-time whole-body overlap check remains mandatory.
- Public analysis seeds reproduce the program without private captures. They
  request analysis without creating new function boundaries. The regeneration
  script also supplies both reviewed timing selector lists.
- Strict comparison checks every ordered picture and all checkpoint WRAM,
  VRAM, OAM, palette, register and clock data. SRAM/APU dumps are outside this
  comparator's scope. A refused requested QoL feature now fails qualification.

New native coverage includes room/actor loading, actor updates and additional
combat, boss and progression helpers. All 190 previous native variants remain.
The new program has 121 declared interpreter variants and 215 analysis roots.
The existing opening-dispatch interpreter repair remains essential.

## Validation

- **95/95 strict replay comparisons**: 73 room entries, 21 combat/endgame
  fixtures and the original fresh-start first-boss route.
- **135,478 ordered presentations and 1,704 complete checkpoints** match the
  prior selected control, including memory, registers, guest clocks and beam
  position. No comparison masks or diagnostic runtime deny file are used.
- **44 project Python tests**, the production QoL C test, and **23 focused
  Python tests plus the C mode-parser test** on a fresh framework reconstruction
  pass. All seven protected owner save/settings files remain byte-identical.
- All **79 fresh canonical coverage captures** were collected and ingested by
  the SDK with zero journal loss or audit warnings. On the same fixture set,
  interpreted instructions fell from 824,526,719 to 779,504,296 (45,022,423 fewer).
  This measures these assisted fixtures only.

The accepted tests use frozen inputs and copied states, including disclosed
teleports, player-stat cheats and prerequisite flags from the campaign batch.
They are separate assisted encounters, not an uninterrupted natural playthrough.
Original attacks, damage, death, rewards and progression outcomes are retained.
The full credits sequence and script-only cloud entry remain unqualified.

The fresh upstream framework patch reconstruction matches all 47 affected
source paths. Focused tests cover instruction ownership, native/interpreter
call boundaries, exact-mode exclusions, root-file generation, cache binding,
strict replay comparison and the camera guard.

Selected executable SHA256:
`864bcaf8dbb4b4c9f2d0ab8f54cb00e5d4c4bfa54162eba6419f3c391e8ab495`.
Program manifest SHA256:
`8b783324ff54638f2fe92dd97d0a1474db0bd267dae5c2c64d9a024495bf72ac`.

## Scope

Launch with `Start Lagoon.cmd`. Owner saves/settings are protected. Source
publication excludes ROMs, generated C, raw captures, save states and audio.
There is no public binary release in this batch. The custom Atland renderer,
widescreen artwork and Enhanced pixels retain their existing scope; this batch
makes no new locked-60-FPS or complete-game rendering claim.

See [timing contracts](tools/NATIVE-TIMING.md),
[generation policy](recomp/README.md), [coverage workflow](tools/CAMPAIGN-COVERAGE.md)
and [current status](docs/DEVELOPMENT-STATUS.md).
