# Native death/revival and widescreen batch 13 — October 17, 2026

The selected program has **346 native variants**, up from 341, with 123 declared
interpreter variants and 232 analysis roots. All 341 prior native modes remain.
The interpreter safety net, camera exclusions and 25 timing/audio deferrals stay.

## Original death and recovery code

The original `$01:92A0` death leaf now runs natively, including the unusual
two-PLA escape used by Thimale's six guardians. Source-proven generated code
performs the original stack operations; no host flag or fabricated death result
replaces them. The six actual guardian deaths preserve full checkpoint WRAM,
register PC/SP, guest clocks and ordered pictures. Parent `$01:9266` remains
interpreted so the existing paired-call ABI owns the inherited return seam.

New native modes also include the revival wrapper `$01:D481`, MP recovery
prefix `$01:D4B1` and its `$D468/$D4AB` helpers, all in M1/X0. A specific
source-reviewed normal-return contract for `$01:940D` closes its HUD case-18
digit/queue chain; the generic dispatcher retains its conservative contract.

Recovery still tiers down at the original cooperative wait. Tests observe
hundreds of MP loop iterations, while the newly native prefix executes only
before the first yield. This batch does not make those full loops native or
claim that every new mode has measured execution. Private extra continuation
and SPC-poll seeds compiled but did no additional native work, so they are
excluded from the selected policy. Native promotion requires measured execution
and proven guest stack/return contracts, in addition to successful compilation.

## Widescreen status and fades

Room 01's status menu and earlier completed destination fade now retain wide
scenery. The missing resource witness was the room's original two-frame torch
animation, rather than menu-font ownership. The exact descriptor, source pointer
table, completed 384-byte DMA and all twelve character bytes must match the ROM.
Partial/wrong uploads, changed tables, unexpected writes and timeline changes
still refuse ownership. Cave torches retain their separate original policy.

The final executable passes **2,112 native-center picture comparisons and 13
complete checkpoints/clocks** on walking/status/close/exit and mid-menu state
reload routes. Independent source decoding verifies margins before, during and
after status. The walking route gains **79 valid wide fields**: 34 status fields
and 45 destination-fade fields, with no previously wide fields lost. Fifty
post-load fields confirm stale room resources do not survive a timeline change.
All seven helper/real-PPU tests and the separate cave regression pass.

Equipment/Save layouts, other areas/effects and remaining transition gaps still
need their own source and runtime qualification. Custom rendering retains its
existing stable-Atland scope. There is no new locked-60-FPS claim.

## Verification and source

Actual execution coverage attributes 194 fewer interpreted instructions to the
original death leaf, one to the revival wrapper, and 120 to the MP recovery
prefix across the 26 combat/primary comparisons. One first-boss scenario appears
twice; deduplicating identical inputs gives 301 fewer interpreted instructions
across 25 scenarios. The two newly declared recovery helpers have no measured
savings in these fixtures. These are bounded execution counts, not speedup or
completion percentages.

The separate 79-capture canonical stock-ROM set was collected and ingested by
the SDK: 779,468,826 interpreted instructions and 2,617,679,029 interpreted guest
cycles, respectively 168 and 654 fewer than its matched 341-variant control.
All four loss counters are zero, capture IDs are unique and warnings are empty.
The different totals reflect different fixture sets; they must not be added.

**99 strict replay pairs pass: 151,417 ordered presentations and 2,657
checkpoints**, including framebuffer, WRAM, VRAM, OAM, palette memory, SRAM,
registers and guest clocks/beam position. Room 01 uses the retained matched
narrow pair for native parity; its wide changes have the separate source proof.
All **48 project Python tests**, seven helper/PPU tests and the camera build
guard pass. Seven owner save/settings files remain byte-identical.

The strict comparator additionally binds optional APURAM, DSP-register and PCM
captures by content and run-relative output path. Missing, partial or differing
declared captures fail. Streaming hashes keep large traces out of memory.
An accelerated PCM experiment failed even against the same unchanged executable:
the current DSP-output capture reads the host ring after overflow discards.
That experiment is retained as a capture limitation, not audio parity evidence.
No playback behavior or SDK code changed in this batch.

The [map reference index](docs/MAP-REFERENCES.md) links all 19 supplied area-map
pages for planning routes and landmarks. Runtime IDs, bounds and rendering
authority still derive from the original ROM. Map crops alone do not establish
the identity or reachability of records 2A/3F, which lack ordinary incoming doors.

Executable SHA256: `16a1bf96b3b2d2a23ecb8e11cb9eab287e4b74d22847fa30986ee105db4d04d5`.
Program manifest SHA256: `27a7dece0b2583433f4495126d5d9c1735e3866790774cf8c5ec496c07366d2a`.
The actual regeneration script reproduces the same manifest and bank/dispatch
C without private profiles. The exact tested executable is selected unchanged.
An independent audit verified every comparison and receipt binding. A further
cold guardian replay using the selected public-build executable passes 4,681
pictures and 110 full checkpoints; this extra check is outside the 99-pair count.
Launch `Start Lagoon.cmd`. No binary release is part of this source batch.

Fixtures disclose teleports, item/stat setup and prerequisite flags. They run
original damage, healing, consumable use, deaths, rewards and progress. They
remain assisted checks rather than an uninterrupted natural campaign, full
credits, independent-console or complete APU/audio qualification. ROMs, generated
C, map artwork, audio, saves and raw captures stay outside the public repository.
