# Second native gameplay batch - October 15, 2026

This supersedes pending items in NATIVE-PROGRESS-2026-10-15.md. Source remains
a hybrid native/interpreter port. No full-game or pure-interpreter parity claim.

## Accepted native work

| Entry | Mode | Work |
| --- | --- | --- |
| 01:C579 | M0/X0 | Sprite graphics staging from ROM to WRAM |
| 01:C5ED | M1/X0 | Transient knockback flags/counter/velocity |
| 01:8385 | M1/X0 | Map/attribute quadrant sampling |
| 02:8199 | M1/X0 | Jump eligibility and input checks |

These join the first batch's four roots. Generation reports 151 AOT-eligible
variants and 18 declared LLE variants. Other modes/unknown entries retain
fallback; the repaired dispatcher at 00:80E6 remains interpreted.

The earlier graphics-staging failure had two causes: a long indirect pointer
was fetched twice while constructing one effective address, and precise AOT
instruction boundaries could leave the PPU beam behind the interpreter at a
deadline. The emitter now materializes the pointer once and carries indexing
across the full 24-bit address. Opt-in precise roots commit refresh, PPU and
coprocessor timing at instruction boundaries and return paths.

00:8211 OAM staging was tried and rejected after CPU/clock divergence despite
matching images. 02:81DE remains interpreted because its call continuation is
unproven. Neither is counted as a native promotion. The earlier whole-port
native/pure-floor differences remain open.

## Runtime evidence

The stock/default-off candidate matches the previously accepted first-batch
native control over 7,219 frames and 18 checkpoints. Complete framebuffer,
WRAM, registers, VRAM, OAM, CGRAM, master clock and beam position match, as do
all 7,219 presentation CRCs. Tests use dummy video/audio, copied configs,
private mod catalogs and fresh private save roots.

Final integration also compares all nine QoL features with and without 16:9:
all 18 snapshots match, including complete native-center pixels and guest state.
The renderer splits shadow tile spans at native borders, preserving the guest's
own edge pixels when camera scrolling lands between tile boundaries. Its actual
PPU regression covers all eight scroll phases and main/subscreen rendering.

Interpreted instructions on this exact route: **70,655,953 -> 63,863,965**,
6,791,988 fewer (9.61%). Relative to the pre-first-batch control, 72,427,712,
the two batches remove 8,563,747 instructions (11.82%). These are interpreted
work measurements, not native completion or wall-clock performance claims.

With all nine QoL switches enabled, the route reaches town and traverses the
movement/menu checkpoints. New-game HP/DEF is 18/11 versus 10/5. A 30-frame down
press advances 75 pixels versus 45, and the next 90-frame down press advances
225 versus 135. Movement applies on alternating game frames. The centered
camera follows actual player coordinates while respecting room bounds.

Town's DP51=82 disables jumping. tools/script_jump_fixture.txt explicitly
sets only that eligibility bit, then uses ordinary A input. This targeted test
preserves the original peak and delays first descent and landing by eight
frames with jump_hold. A moving jump covers twelve more pixels before landing.
The default-off fixture also matches the prior native control at all 137
checkpoints, including CPU and clock, so the promoted airborne branches are
covered in this targeted case. It is not evidence of reaching a jump-enabled
map. The existing gameplay
route's jump-named tags alone never proved airborne movement.

All four fixed widescreen aspects passed 18-checkpoint stock guest-state and
native-center parity. Default CRT pixel shape gives source widths 342, 308,
448 and 682 for 16:9, 16:10, 21:9 and 32:9. The independent retained-map audit
matched 23,040 tile entries. Expanded backgrounds are real world reconstruction;
off-screen entity composition and parallax remain unfinished. See
docs/WIDESCREEN.md for the renderer policy and combined-feature checks.

## Implementation and verification

All nine QoL features are default off and selectable in Companion. ROM changes
touch guarded data, not instruction bytes; active mechanics share a canonical
CPU WRAM-write filter used by both execution tiers. Byte/word aliasing, ordered
low/high writes, WRAM wrapping, bus boundaries, original open bus, non-WRAM
bypass and hook removal passed against the actual cpu_state.c implementation.
Faster walking uses a Nasir-only filter; the original shared speed table remains
stock, so enemy/NPC tier-two movement and three-pixel followers are not accelerated.

Actual-source QoL tests cover stock identity, data guards, starting/loading
records, follower exclusion, eight-tick holds, callback order and reset/load
generation changes. Remaining hold time is not serialized into SNES snapshots;
loading mid-hold cancels stale host counters and resumes stock motion/expiry.
Exact mid-hold snapshot and rollback parity is unqualified.

Companion's native display/audio/gamepad controls, nine QoL switches and
experimental aspect choices passed Apply/Revert, stale-edit, missing-package,
rollback-on-failed-commit and unknown-setting/binding preservation tests. The
six-page UI was rendered and reviewed, and a private 120-frame native launch
passed. Existing owner configuration and saves are not migrated.

Three precise-emitter checks, fifteen upstream cycle-model checks and the CPU
WRAM filter regression pass. A fresh upstream worktree accepted the complete
framework patch and passed its emitter/filter regressions. Upstream base remains
a6be9756285f26ccc74539c811d4d23d57d8c33d; the public submodule pin stays at
that reachable commit and patches/lagoon-framework.patch carries local changes.

Before testing, 139 pre-existing save/state files were backed up and hash
verified. All 139 hashes remain unchanged. These include old development states,
not 139 player save slots. Source publication excludes all ROMs, generated C,
private decompilation exports, saves and captures.

Private evidence: workspace work-native-20261015-111814/, notably
final-native-verification.json, ready-off/, ready-all-qol/,
ready-wide-all-qol/, jump-fixture-verification.json,
widescreen-verification.json, wide-map-verification.json, launcher receipts
and the framework reproduction worktree. No playable binary release is published.

Next priorities: actual combat/follower/spell progression, wider native hot-path
coverage, off-screen widescreen entities, and exact optional timer serialization.
