# Development

This is a mixed native/interpreter port of the clean USA game. Qualified 65816
routines are compiled to C; unqualified modes, variable scripts and special
hardware paths retain the original-opcode interpreter. Do not replace a fallback
with an empty function, guessed event result or synthetic return.

## Current scope

Source 0.2.2 includes CompanionAchievements and a small host-side hidden encounter,
targeting `v0.2.2-preview.1`. The interpreter safety net remains part of this
release. The native entry counts below describe the preceding qualified native
batch, rather than a new claim of complete native coverage.

The 0.2.2 renderer avoids recalculating all 128 sprite placement policies on
every later raster line when their exact inputs are unchanged. Changed inputs
still execute the original full calculation. Background planes can also reuse
their source-decoded colors and depth when their complete inputs match; actor
and HUD composition stay fresh, and the full reference-picture gate remains.
Width, generation, maps, definitions, VRAM, palette, scrolling and mosaic changes
invalidate reuse. This grants no new graphics ownership. Focused background
benchmarks save about 56-62% of that renderer phase with a stationary camera and
changing actors, while scrolling stays effectively unchanged. The policy
observer saves about 0.040 ms per field. These are CPU headroom measurements,
not a promise that every wall-time spike or owner-reported flicker is resolved.

A separate source/mode/physical-frame/generation-qualified native service runs
the finite inactive actor-slot iterator in selected original scripted rendering
waits. Every active actor call, hardware owner and final rendering sequence
remains original. Per-opcode bus work, flags, open bus and CPU/master deadlines
are preserved. It is deliberately refused on incompatible timeline contexts;
ordinary walking uses the already-qualified main actor loop. Its live disabled
versus enabled cold/menu control matches 2,466 ordered pictures, complete
checkpoints and audio captures. Generated entry counts below are unchanged.

The 0.2.1 encounter fixes were checked from an empty save directory through a
new game using SDL's virtual gamepad and the normal controller mapping. Checks
cover approaching from both sides, collision, walking around the NPC, completing
and cancelling dialogue, actual WASAPI sound playback, a single secret unlock
and movement afterward. The fresh-game dialogue font check binds only the
required glyph and border tiles; unrelated unused BG3 art no longer blocks it.
Collision filters the original movement operands with exact caller/mode/source
guards and retains the original movement, camera and guest state ownership.
These focused checks do not replace the owner campaign playtest.

The reported Atland house/door flicker remains open. Focused source/headless
checks across the northern and southern streets matched native scenery and
door sprites, but did not reproduce the fullscreen/enhanced presentation issue.
No renderer guards were relaxed on the strength of those negative checks.

A separate shared OAM motion-history defect was reproduced in the actual PPU:
a large jump into an offscreen parking position inherited up to three fields
of left-margin visibility. Large discontinuities now clear that history;
small movement and briefly stationary sprites retain their existing grace.
Boundary tests also preserve legitimately positioned sprites at the same X
coordinate. This fixes a shared visibility rule, not a blacklist of doors or
Atland coordinates. Four walking passes through Atland with three reversals
matched before/after in 61 source frames and three actual hidden OpenGL/EPX
readbacks; that route did not exhibit the owner's remaining flicker. The older
gallery image's detached door and uncovered doorway were already absent from
the current matched view; the gallery now has a current 1920x1080 capture.

Scenery contracts cover all 75 authored room descriptors. Native work includes
player damage, death/revival, recovery/reward loops, boss states, Bird/endgame
services, sprite staging, resource/tile queues, numeric UI helpers and original
display fades and queued graphics transfers. The HUD-number dispatcher has a
separate, source-guarded handoff
using its actual cross-bank JSL return frame. Counts of
compiled functions are analysis counts, not completion percentages. The qualified
native foundation has 654 AOT-eligible entries, 74 analyzed interpreter entries
and 508 roots. These are exact entry variants, not a native completion percentage.
Boot audio and unqualified script/loader contexts retain original opcodes.
Generated code alone does not prove that a runtime dispatcher reaches
a compiled entry.

The original 34-slot projectile animation table has qualified M1X0 leaf handlers
and its shared return tail. Its effect dispatch now also qualifies all six
distinct source handlers, including homing and fork allocation. The complete
effect closure has 15,104 original-opcode cases; actual casting routes separately
verify native handoffs for the default effect and homing. Other modes and unknown
pointers retain the interpreter. The outer projectile loop now qualifies M1X0
after complete source-opcode tests and matched live floor/native controls for
default and homing casts, including ordered APU events and all checkpoint files.
An earlier candidate failed the audio gate. The absolute-periodic APU timeline
now avoids an extra interpreter-only WRAM-progress flush, sharing the completed-
opcode threshold across tiers. Boot and legacy progress flushes, port-access
flushes and exit/deadline flushes remain. Other outer-loop widths and unknown
returned modes retain original-opcode fallback, including the overlapping RTI
entry. Passing memory and picture checks alone is insufficient.
The finite animation table has its own separate original-opcode tests.

The original enemy proximity/retirement service now includes the qualified
M1X0 command at02:824A and its actual return. The bank-request family preserves
its real cross-bank return frame, saved data bank and original source targets.
Unknown modes and pointers remain interpreted. This preserves the original
record clearing, multiplication and possible actor reinitialization; retirement
is not replaced with permanent deletion. The full ordinary actor depth-sort and
draw iteration loop is also compiled in its qualified mode, retaining the
original dynamic draw selector and its real child handlers.

The ordinary enemy projectile-request dispatcher has a separate M1X0 native
closure for ten authored enemy kinds, including facing shots and projectile fans.
It keeps the inherited two-byte JSR return owner, original sound commands and
first-free projectile allocator. The original indirect pointer-tail site admits
only its qualified target; other pointers retain the original interpreter tail.
The adjacent alternate dispatcher, unknown modes
and variable script services retain the interpreter. Instruction timing for the
shared sound parent and its two port-writer children is qualified separately;
this does not qualify every soundtrack descriptor or upload path.
Source-opcode checks include the actual ancestor dispatcher and unknown pointer
boundaries. Live controls separately verify native activation, real projectile
allocation and exact ordered APU events, registers, physical state and clocks.

The item-use request and selection prefix preserves its actual caller's
two-byte return frame. The inline helper consumes only its real three-byte JSL
frame; six exact table/parent/physical-stack contexts can then dispatch directly
to available qualified native handlers. The families are item use, magic,
death, two boss-script tables and 40 source-qualified NPC script commands. Other table
words, source mismatches, modes and return owners fall back before mutation.
Unexpected native return-stack violations terminate with a contract diagnostic
instead of fabricating a return. The generic ExecutePtr lowering remains
inappropriate for Lagoon's ABI.

All eight authored magic table handlers and their finite debit/spawn helpers
have original-opcode checks; live controls separately exercise actual spell8,
real L-button Shiny Ball use and the retirement route. Source-only tests of
eight music/script/query parents do not establish natural quest activation.
A default-off, source-bound diagnostic fixture now aliases only the original
NPC script pointer after a lawful NPC 0 interaction. Commands 30/39 each reach the
native dispatcher and their original CCD8 return: 81 pictures and two checkpoints
match the same-build interpreter controls. Only two script-input bytes change;
PC, physical frames and handler outcomes remain original. The earlier malformed
NPC 80 attempts remain negative evidence. Assisted activation is distinct from
natural quest progression, and does not establish live coverage of all 27 commands.
That 0.1.9 result covered 27 commands; the 0.1.10 qualification below extends the
family while retaining the original dispatch for unknown contexts.

The verified development0.1.8 batch retains exact clock/physical state checks.
Source checks cover15,720 bank-dispatch cases,
4,096 parent cases,60,480 inline guard cases,11,968 magic cases and640 actual
actor iteration cases. Counts describe bounded tests, not campaign completion.
A matched native/interpreted-magic route preserves716 ordered pictures, eight
checkpoints, all audio queue targets/application clocks, SPC memory, DSP
registers and PCM bytes. A separate read-only buffered queue observer verifies
actual device application time; old captures without it cannot pass that gate.
The legacy raw strict comparison is retained unchanged, including failures.

The 0.1.9 music work adds three source-owned native port writers for FE00/FE2F/
FE5E. They publish the port access at the same opcode-start CPU/master point as
the original interpreter, then charge the original instruction total. The real
RTL frame, mode, source, caller and generation remain guarded; unknown contexts
use original opcodes. This changes no shared clock policy or guest duration.
In that batch, the three complex upload/handshake paths FE9B/FEEC/FF69 remained
interpreted until their device-event timing was qualified. The three writers pass 38,412 original-
opcode cases, including ordered port observations, deadlines and physical returns.
A fresh control excludes exactly the five remaining native music entries. The
81-picture NPC music route and 716-picture spell route match full checkpoint
state/clocks, actual queue targets/application clocks, DSP registers and PCM.
All three native port-writer counters are positive; the control counters are zero.
The legacy raw trace still differs in deferred-apply CPU-beam/sample observations
and retains its failed strict result. The separate device comparison passes;
its exact timestamps are not inferred from those observation fields. The final
0.1.9 rebuild matches the qualified candidate on both routes.

Development 0.1.10 qualifies the remaining 13 NPC command contexts, completing
the 40-command family in its exact source/mode/physical-frame contract. The
source oracle binds 198 generated closure bodies and checks 28,864 cases;
49 typed dialogue entries have another 21,952 original-opcode cases. Real input,
NMI and child-call waits remain intact. Newly exposed continuations without a
separate source proof are explicitly interpreted. Assisted dialogue and music
inputs are diagnostic fixtures, not evidence of natural quest completion.

Native music now includes guarded bank, sample and transfer services, plus a
source-tested port toggle. A fresh assisted song-bank change matches all 62,434
queued requests and applications, CPU state/clocks, DSP registers and PCM.
Fresh boot, dialogue and spell controls also pass the device gate. The initial
unmapped upload remains original: its interpreter owns private boot/progress
accounting. Native services require the mapped absolute audio timeline and
preserve its original pre/post port synchronization and atomic word accesses.
The toggle has source tests but no known authored activation witness. Earlier
failed cold controls remain recorded; the separate device result never rewrites
the legacy raw observer comparison.

Moving actors can now use a verified completed original draw packet rather than
an already-advanced CPU pose. The packet binds the original stable sorted queue,
OAM count, full OAM bytes, auxiliary pose data and resource/generation ownership.
Unknown writes invalidate it. A moving Voloh NPC has independently checked
sprite and foreground-occlusion pixels in the added view. Endgame Thor, Bird
and orb contracts also bind their original creators and draw packets. The real
arena clamps these actors inside the original view; separate synthetic seam
tests are not presented as natural far-margin coverage.

Compatible Voloh loads now recognize the exact original common2-then-common3
resource history. Common3 expands 1,476 bytes but uploads 1,728, including bytes left
by common2. Both streams, original predicates/writers, all 10,752 ordinary atlas
bytes and the complete common upload must match. Its 1,216 bytes outside the
ordinary atlas remain unowned. The new scope has original C1C6/C026 NPC cells and
independent planar-pixel checks, plus an assisted stationary NPC 47 case. The
completed-packet witness above also covers verified moving poses; unprimed or
unqualified forms still decline. This is a specific resource combination,
not generic follower or natural escort coverage.

Widescreen presentation now gives completed source-qualified menus priority
over world caches. Qualified incoming all-background mosaic packets preserve
the original256-pixel center and add margins only after every224-line center
word matches. Unsupported packets retain safe original output; an incoming
scene never inherits stale outgoing scenery over its center. The shared PPU's
derived mosaic lookup is refreshed at the first visible line, supporting beam
hosts that render1..224 without line0. Pre-render flags and guest timing are
unchanged. Both PPU backends have independent scalar mosaic checks; an actual
14-to-2A transition verifies44 mosaic frames with zero center mismatches and
immediate widescreen on the first clear frame. Earlier broad checks cover eight
room families and three menu panels, separate from this final affected route.

A focused246-field hidden1080p OpenGL/EPX/original-audio enemy walk preserves
all guest periods, nonwall gameplay observations and12 checkpoint files.
No interval exceeded20ms in this check. The native actor loop saves a little
host work against its same-build interpreter control; whole-route work is
close to the previous build. This does not establish a global performance
improvement or a locked60Hz guarantee.

The 0.1.9 read-only character-discovery scan visits each metatile once and reads
only its covered tile definitions. Across 75 original room maps, source reads
fall from 7,407,646 to 660,398 (about 91%); 1,195 cases compare each plane and final
used-character bits, including odd edges, fill, parallax and changed sources.
The upload/VRAM/torch ownership checks are unchanged and no persistent authority
is cached. A qualified acquisition candidate was measured on a matched 97-field
original 14-to15 OpenGL/EPX/WASAPI route: total guest work changes 127.91 to
124.33 ms; loader field 83 changes 2.54 to 2.43 ms.
Both captures retain two intervals above 20 ms and the same original guest periods;
maximum intervals are 22.36/22.30 ms. This is bounded extra CPU headroom, not an
all-area speedup or removal of every spike.

The assisted campaign work uses original loaders, doors and event/boss scripts,
with position/stat assistance to reach affected cases quickly. It is distinct
from an uninterrupted playthrough. That playtest belongs to the owner and friends.
Earlier checks are not a reason to rerun the entire campaign for every change.

Remaining work includes unsupported special/multipart forms, dynamic graphics,
magic effects and display layouts; additional native variable-script services;
frame-time spikes; and bugs reported during real play. Source-qualified native
enemy arithmetic helpers now qualify all63 ordinary room descriptors00–3E,
with exact descriptor/source/mode/real-stack guards. Native work includes tile
sampling, movement integration, offscreen projection and visible OAM cell packing.
Ordinary enemy direction/velocity commands have a separate guarded host handoff
that stops before the original RTL. Original AI, collision, draw streams and
hardware multiply remain in their original owners. Pure host calculations
intentionally remove guest computation cost; their opcode oracles prove results
and physical-stack semantics, not identical guest instruction timing.

A focused assisted Elf Field route improved from631 to716 logic updates over
1432 fields: the intended30Hz game logic at60Hz display cadence. The final GPU
and original-audio run is measured separately from this headless cadence check.
These are affected-route results, not all-room performance or campaign coverage.

Built-in Giles/Thor escort following
uses bounded local A* toward a recent Nasir position instead of replaying an
old route. Four-pixel cardinal edges check every pixel against the original NPC
leading-edge probes and current eight-pixel collision attributes. The navigator
accepts source tags0/3/5 and rejects solid80, tags1/2/4 and unknown tags. A search
examines at most4096 nodes in a512px local window; cached routes are checked again
before movement and replanned after blockage, target changes or load. Follower
speed is capped by Nasir's observed walking pace. No accumulated catch-up speed.
Source/mode/real-stack/generation-qualified pre-opcode hooks retain original facing,
velocity computation, tile collision and integration. Only intent and bounded
velocity change; positions, quest flags and doors are not synthesized. A separate
source-qualified80C7/A110/80CA read view makes only a unique ordinary quest escort
nonblocking to Nasir, including before trail bootstrap. It changes no stored actor
flags or return frames; original scanner skip branches execute. Exact interpreter
PC, actual80C9 frame, mode, scene, tick and generation are mandatory. Unknown
native scanners decline; promoting that scanner requires preserving this policy
with new source evidence and tests. Scripted
or unqualified roles retain the original path. Earlier internal fixtures named
"Ella" actually instantiate source NPC kind0, Thor; they are not evidence of
natural Ella campaign coverage. Disposable history resets on
loads/rollback; run-ahead declines speculation during escort quest flags.

The built-in explored map reads original area names, dimensions, door links and
treasure records at runtime. It reveals only nearby observed cells, uses source
collision bit0x80 solely for conservative solid shading, and shows chests only after
their cells are discovered. Other collision tags are not walkability authority.
Opened markers follow original persistent flags. Eight original door records lie
outside their room's declared extent and are omitted from markers. The shared
host's optional title panel freezes guest/script execution, audio production and
run-ahead while presenting the last world field. Exploration persists separately
from guest saves, with validation and atomic replacement. Tablet-specific hints
read the original Thor/elder branches, item bits and chest data. They show the
next instruction and applicable marker without writing items, flags or fog.
Broader quest guidance, a full objective journal and a possible15–20% zoom-out
remain later work.

The title-owned quick-settings panel shares the frozen-overlay host and edits
master volume or replacement-music trim, with an entry for the read-only
achievement browser. L2 opens the map; R2 or Ctrl+comma opens settings; Ctrl+J
opens achievements directly. Native controller callbacks keep these extra triggers separate
from the twelve SNES inputs. Companion also exposes keyboard, controller and
system-hotkey editors with optimistic revisions and rollback across native files.
Reward selectors scale only the source-qualified enemy/boss gold and EXP words;
shop prices, quest rewards and existing saved totals are not rewritten. Original,
legacy1.5x EXP and2x–8x settings are mutually exclusive within each reward type.
[Mike's RPG Center maps](https://mikesrpgcenter.com/lagoon/index.html#maps) and the
[walkthrough](https://mikesrpgcenter.com/lagoon/walkthrough.html) are route references,
not proof of runtime state or permission to redistribute their maps.

## Renderer

The custom scene renderer reconstructs supported world backgrounds from actual
resident resources. It verifies the original center picture before replacing a
scene and falls back for unknown room/resource/palette/generation combinations.
It does not stretch a screenshot to fill the margins or invent content past
finite world edges. [Room catalog](RENDERER-COVERAGE.csv).

Transition presentation may bridge a qualified room change exactly one completed
field after a coherent full-width canvas, before the original loading flag is
set. Repeated presentation of that same field is stable. Unknown rooms, stale
canvases, geometry changes and state generations refuse the bridge; normal
same-room UI is not covered by this rule.

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
offset before the original post-draw update, including the source-proven first
activation field, with the complete native OAM match
still required. Its terminal field can retain the last draw offset after the
original update clears the shake state. Current cells take precedence; a terminal
candidate is used only when its complete group matches original OAM and the same
resource and camera guards. Unsupported forms, unowned optional layers and
Nasir's dynamic graphics retain fallback. Current and retained packets are
withdrawn in the same field when an upload replaces resources they own.
Queued OAM ownership, priority, clipping, palette, camera
coordinates and resource generation must all match before extending a pose.
Changing a guard requires source evidence and affected-route checks.

Title/intro/portrait UI can retain original geometry. Qualified dialogue and
Start views retain their surrounding world; audited Items/Magic/Equipment
presentation and transitions use original assets. Scene transitions need valid
resource provenance, not just the previous room's pixels retained indefinitely.
A guarded presentation cache retains a coherent expanded field during supported
partial-loader uploads, applying the current fade brightness. It resets on state
generation changes and refuses geometry mismatches. Other transition paths still
need affected-route checks; this is not a claim that every line, rip or temporary
original-width view in the game has been eliminated.

The sword-reach feature has a separate presentation extension. Source-qualified
sword tiles determine the authored blade angle and a12-pixel tip extension. It
preserves the original blade and observes source depth/priority witnesses, without
redrawing Nasir or changing collision. The visual extension is enabled only after
the guarded38px hit rectangles actually applied.

Replacement X68000 music uses per-cue attenuation toward a measured loudness
reference, with an independent0–100% music trim. The original PCM samples, pitch,
loop headers and SNES sound effects stay intact. Existing clipping in supplied
samples cannot be reconstructed through playback attenuation.

## Qualification

Achievements observe completed gameplay changes without modifying guest memory,
clocks, scripts or rewards. An isolated host worker owns the
CompanionAchievements C engine and atomic local progress writes; gameplay queues
bounded facts and the UI reads published progress. The launcher is read-only.
The catalog has 26 entries, including a source-bound actual ending completion
witness after the seven credits scenes and final artwork return; entering a
boss room is never treated as completing the game. Loads/generation changes
clear transient observation witnesses without awarding the loaded snapshot.
Assisted diagnostic sessions disable tracking and notification audio.
Progress uses `<actual-save-root>/Achievements/lagoon/native-qol/unseeded/`;
storage failure or an existing writer disables tracking without blocking play
or replacing the owner's progress. UI panels pause the guest and consume opener
and close-button input before gameplay resumes. The unlock sound has a separate
host stream and follows master volume; it never touches the SPC driver.

The tiny credited cameo uses host-side presentation and interaction without
creating original actor, item or quest records. Its NPC clearance uses a guarded
read view in the original player movement path. Its four source GIFs and
decoded pixels have a separate copyright provenance record in
`assets/easter_egg/provenance.json`. They are not Lagoon ROM extraction and are
not covered by the MIT notices for the achievement toolkit. Keep the hidden
location and discovery text out of player-facing instructions.

Keep source, architectural mode, actual guest return-stack and generation guards.
Native promotion is checked against original opcodes with complete memory,
registers, stack and CPU/master/coprocessor clocks, including internal deadlines.
Use matched inputs and controls for affected gameplay, ordered presentations and
complete checkpoints. A matching screenshot alone does not establish correctness.
Timing, audio, scheduler or dead-stack differences must remain explicit.

The APU batching change also changes legacy trace observation points versus older
builds: some device events carry the CPU beam position at flush time, and CPU
requests observe the current output backlog. Those old/new raw traces are not
byte-exact. Separate real SPC/DSP tests compare complete state, scheduled ports
and non-silent PCM across chunk sizes; final same-build native/interpreter game
controls additionally require the unmodified strict trace comparison to pass.

See [native timing contracts](../tools/NATIVE-TIMING.md),
[analysis policy](../recomp/README.md) and
[bounded campaign warps](../tools/CAMPAIGN-WARPS.md). The cheat hook requires an
isolated bounded offline run with dummy audio, no run-ahead and an actual original
door handoff. Never relax those production guards to obtain a test result.

Frame-time measurements must distinguish normal guest work, rendering,
per-frame diagnostic file I/O, frame limiting and audio high-water waits.
Loading an assisted state with a full saved audio queue can briefly wait for the
real device to drain it. That wall time is not a native CPU execution cost; keep
the audio wait counters alongside the guest timing rather than discarding PCM.
Dummy-audio/hidden-window results do not certify real-device audio or scanout.
Fresh room-loader tests and loaded-state tests differ: loaded states do not
serialize the custom renderer's resource-ownership witnesses. A successful full
load can requalify the complete resident ordinary-actor atlas for room08/09 only
when its original descriptor, compressed sources, background-derived tail and
complete VRAM bytes all match. Actual Elf Field load checks show added far actors
with an exact original center. A second Voloh contract proves the original
common2 upload that overlaps the final512 bytes of its room atlas, including
the complete1760-byte upload and the original loader/predicate/source bytes.
Unsupported overlays and changed bytes refuse ownership. Actual NPC margin
pixels have independent planar-decoding and live center-picture checks.
Other resource families still require their original upload witnesses. Re-enter
through an authored door when testing those families.
The common0 enemy-death glyph pack has a separate successful-load recovery:
its original loader/decompressor/queue/NMI sources and all 512 resident bytes
must match. It uses the same one-attempt load gate and generation invalidation;
partial transfers and unknown writes cannot acquire ownership.
Equipped OBJ magic has its own successful-load recovery for the complete original
pack, with exact source, room, form, generation and resident-byte checks. Its
cells use only that independently verified pack; an unowned ordinary room atlas
cannot lend them tiles. Ordinary enemy projectiles still require their room
atlas, and every magic packet retains native OAM, ordering and cell-ownership
checks. A loaded homing-spell route verifies actual margin pixels and the original
center independently. Fullscreen magic and multipart resources retain their
separate contracts; unknown writes and partial packets still refuse ownership.
Room $14 has a separate, exact successful-load actor-pack contract. The original
stream supplies 8,324 bytes, but its larger DMA transfer contains an unknown tail.
Recovery checks all source bytes and authorizes only 8,320 complete tile bytes;
the remaining tail stays unowned. Live checks identify a specific previously
missing enemy in the added view, with independent planar decoding and an unchanged
original center. Other rooms do not inherit this authority.
`SNESRECOMP_GL_TIMING` optionally records buffered CPU wall times for existing
upload, draw and swap calls without extra GL work. Verify the initialized backend:
an explicit `Renderer=auto` overrides `OutputMethod=OpenGL` and selects SDL, ignoring
the Enhanced shader. Use `Renderer=opengl` for actual Enhanced-path measurements.
There is no full-game locked-60-FPS claim.

SDL3 host pacing uses its precise nanosecond wait for the complete remaining
deadline budget, while retaining the SDL2 wait policy. This follows
[SDL's timing guidance](https://wiki.libsdl.org/SDL3/SDL_DelayPrecise); operating
system scheduling can still delay it. Focused measurements show a modest
loaded-room improvement. Longer intervals requested by the original guest's
frame periods remain intentional; this change does not shorten game time.
The optional buffered frame trace also records requested wait budgets, late
returns and wait-call counts. The timing summary preserves the previous guest
period alongside each interval, distinguishing scheduling context from observed
host lateness. These columns overlap wait time and must not be added to it or
subtracted to hide spikes. Recording adds no clock, SDL, GL or per-frame disk calls;
it is diagnostic instrumentation rather than a new performance guarantee.

VRAM observers now acknowledge a generation reset once while leaving resource
ownership empty. Previously, clearing a cache also cleared its generation marker,
causing repeated large clears on unrelated VRAM bytes. Inactive fullscreen-spell
state also avoids repeated identical clears. Exported reset behavior, partial-DMA
handling, source checks and unknown-write invalidation remain intact. Differential
tests compare reachable cache states and ownership decisions; the only intentional
metadata difference is acknowledging the empty generation. A focused loader check
reduced guest-work wall time while preserving every guest period and final APU
checkpoint. Longer intervals requested by original guest scheduling remain, so
this is not a claim that every transition now presents at a locked 60 FPS.

Successful user loads on the restored absolute APU timeline now clamp obsolete
host-output backlog to the existing2136-sample cushion after title execution
reconciliation. SPC/DSP synthesis state, clocks, snapshot format and future
generated PCM remain unchanged; queued old output and its read cursor differ
intentionally. Rollback, legacy/boot timelines and failed loads retain their
existing behavior. Desktop audio resets once per successful load, while rewind
history still invalidates. This avoids waiting for a full saved8192-sample ring
to drain before gameplay resumes. Device-queue behavior is separate from guest
state and must be qualified as such; do not normalize arbitrary audio differences.

Current upstream runtime fixes are reviewed against this workspace's cumulative
patch. Interrupt-tail ownership from0896251 preserves the original RTS/RTL owner
inside interrupts. The consumer-progress policy from65b9387 rearms a stalled
producer only after actual PCM consumption or low queue occupancy, preventing
callback-only activity from repeatedly charging a250ms wait. The desktop retains
its existing unlocked sleeps and profiling; it does not need the optional shared
host-callback API. The dirty SDK checkout and public baseline pin are preserved.

## Build and contribution

[Build instructions](BUILDING.md). Keep the framework patch and dependency licenses.
Apply/check the cumulative patch on clean checkouts; preserve intentional local
framework changes in active workspaces. Never reset the SDK or accidentally
commit a dirty submodule pointer. ROMs, ROM-derived generated C, decomp exports, states,
extracted Lagoon assets and raw validation stay private. The tiny credited cameo
is a separate, explicitly requested third-party artwork inclusion. The owner-approved converted
X68000 album and its MP3 listening exports are the explicit audio distribution
exception; other private audio remains local.

Keep changes focused and report what actually ran. Source tests, synthetic
fixtures, assisted routes, installed-artifact checks and natural playthroughs
are different evidence. Commit/push substantial verified batches. Development
handoffs and per-run receipts stay local; Git history preserves old reports.
Per-symbol notes naming retired batch reports refer to that history, including
the [pre-consolidation source](https://github.com/rages4calm/LagoonSNESRecomp/tree/b04a6300d8da11d5d16d6676b2e8b556ce548aee).
