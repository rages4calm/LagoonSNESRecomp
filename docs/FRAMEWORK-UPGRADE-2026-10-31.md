# October 31 framework integration

Development source now pins SNESRecomp
`98f41838443a7cd7f9d56bec41387a6ebfc3bcbc`, 192 commits after the prior
`a6be9756` release-build pin. This includes the October 30 optimization campaign
and subsequent shared GPU Mode 7 work. Existing Windows 0.2.5 and Linux/macOS
preview downloads are separate qualified artifacts; changing source does not
update their binaries or establish a new campaign playtest.

## Preserved behavior

The residual framework patch carries title requirements still absent upstream.
Fresh application was checked against the pinned tree. Already-integrated
upstream changes are removed from the old patch rather than applied twice.

Preserved contracts include exact source/mode/stack/generation admission,
native/interpreter continuation ownership, instruction/audio timing, WRAM
filters, modal dialogue audio/input, host volume, wide OAM/scroll buffers,
native-center shadow boundaries, first-visible-line mosaic, save/load
reconciliation and profiling. New upstream guest-scope, producer-wait and HD
packet interfaces remain present. Lagoon's custom scene rendering stays
authoritative; GPU Mode 7 does not redraw ordinary tiled artwork.

The build adopts upstream native DSP mixing and fused immutable ROM reads.
References remain selectable. CPU interpretation, SPC execution and all
existing excluded game variants remain supported.

## Initial framework57 qualification

- Initial upgrade generation retains guarded manifest
  `67a87e43c43317eaaf8bc7303bce70c8c6a98a37e9c7ea79aa9af16257fbae11`.
  Game bodies are identical; only generated build provenance changed.
- Framework generation/continuation tests: 616 passed, 2 skipped, 20 subtests.
- Wide native-center boundary: 8 scroll phases, main/subscreen and margins.
  WRAM alias, order, wrap, open-bus and unregister checks passed.
- Matched isolated routes cover Atland 32:9, Atland/cave/Elf/Voloh transitions,
  Elf enemy activation and Evil Spirit/Zerah health fixtures. Ordered pictures,
  checkpoint guest memory/registers/clocks, APU ports, DMA and PPU writes match
  the released Windows control. These are bounded fixtures, not a campaign.
- A separate released-control/new-build comparison retains per-frame ARAM and
  DSP-register differential snapshots. No audio-trace exception was granted.
- Elf combat profiling uses serial ABBA control/candidate/candidate/control
  runs on one Windows machine. Gains are route-specific, not the upstream
  campaign's percentages and not a universal locked 60 fps claim.
- Linux/IntelMac/AppleSiliconMac compilation, GL and isolated persistence gates
  run in the private build repository. No ROM or owner progress is uploaded.
  Those gates are distinct from target gameplay/audio/controller playthroughs.

Compact local source bindings/receipts are in the owner's `validation/upgrade57`.
Raw outputs are private and disposable after summary/fixture retention. The
installed Windows release and original dirty SDK remain intact.

## RomAtlas research and timing follow-up

RomAtlas was built with Rust 1.90.0 and locked dependencies as an independent
research tool. A local Windows adapter explicitly refuses its Unix-only
external assembler execution; inspection/contextual decoding remain available.
No RomAtlas emulator/runtime code is linked into Lagoon. Its combined CLI uses
GPLv3; independent native-port helpers have separate MIT grants.

Two timing boundaries were investigated using its decoder and live coverage:

1. `$00:81F7:M1X0` is a queue service with a real `$00:81AB` child and a wait
   on `$023A`. The original candidate passed 6,144 opcode comparisons but was
   rejected because its per-frame audio snapshots differed from interpretation.
   Completed-transfer timing alone did not fix that. The timing58 follow-up
   below resolves the deadline/audio handoff and qualifies the exact native mode.
2. `$01:814E` is the heavily executed `$0418` interrupt wait. Its counter update
   is in the original NMI route. Generic cycle replay has IRQ/HDMA/epoch guards.
   The bounded probe did not demonstrate replay activation; no declaration or
   guard relaxation ships from that experiment.

`tools/test_shared_services_oracle.py --probe-wait` supports the source matrix
against an explicitly selected SDK (`SNESRECOMP_ROOT`).
`tools/qualify_wait57.py` repeats the strict same-EXE 81F7 comparison with a
matching executable/manifest. It requires positive native-entry evidence,
ordered pictures, full checkpoint state/clocks, per-frame ARAM/DSP snapshots
and the complete captured DSP sample stream. It fails on zero native entries
or a capture fault, even if both output files would otherwise match.

## Timing58: native queue handoff and safe diagnostic capture

Precise native code now flushes elapsed absolute APU time when a host deadline
unwinds to the interpreter scheduler. This also happens when native execution
left no relative interpreter credit. The original interpreter deadline exit
already performed that flush. Periodic pacing, real guest clocks, source/mode/
stack/generation guards and all other fallback contexts remain intact.

`$00:81F7:M1X0` is enabled with precise native timing. The existing opcode
matrix is reused because its generated instructions did not change. Final
Windows checks cover 21 and 170 actual native entries in short/long routes;
both controls match strictly, including audio snapshots and sound samples.
A fresh final-build two-state check adds 47 actual entries and matches through
both loads (114 fields and two checkpoints).
The retained four-door qualification adds 594 real entries, 1,598 ordered
fields and six complete checkpoints, including a second save-state load.
It also matches the targeted same-EXE interpreter control strictly. These are
assisted fixtures, not another campaign or proof of every caller context.

The old-runtime/new-runtime four-door comparison is **not fully identical**:
one recorded SPC event changes its CPU beam annotation from line225/clock52
to line239/clock220. Its value, SPC sample472084, order, complete captured
samples, gameplay state/clocks and pictures remain identical. The strict
comparator still reports this difference; no timestamp exception was added.
The full native/interpreter comparison within the fixed runtime is exact.

The multi-state audio check also exposed an upstream diagnostic bug: the static
capture cursor survived a reset/load while the DSP ring counter could rewind.
Unsigned iteration could then emit almost 16 GiB of repeated ring samples.
Capture now establishes its cursor per state generation before a frame starts,
bounds spans to the ring capacity, preserves valid uint32 wrap and reports
invalid spans/write failures. Qualification rejects those errors. Playback
without `SNESRECOMP_DSPOUT` does not enter the recorder. See the independently
confirmed [upstream report #233](https://github.com/RetroPortingToolKit/snesrecomp/issues/233).

The changed bridge passes 452 ROM-free checks, including the native deadline
flush on/off cases. Capture-cursor regressions cover restore, wrap, overwrite
and an unexpected epoch change. Qualification/comparison/patch lifecycle tests
pass (13 + 28 + 1). Unchanged framework suites from the initial upgrade were
not repeated. The complete residual patch reconstructs the tested SDK tree;
the 115 original dirty SDK files remain byte-identical.

Linux, Intel Mac and Apple Silicon Mac native build, bridge/cursor, GL,
persistence and 20 package-guard checks all passed private workflow run
`41254055403`. It uses
an input-specific build-cache key so a changed frozen source cannot reuse an
older compiled object merely because archive timestamps are old. No ROM or
owner saves are uploaded; generated C remains private. Target gameplay,
physical audio/controllers and campaigns remain separate qualification work.

The timing58 generated inventory was 655 AOT/73 analyzed LLE variants, not a completion
percentage. This does not claim 100% translated CPU coverage or remove the
supported interpreter. Existing installed game/release downloads are unchanged.

## Timing59: reset wrapper and DMA request wait

Two additional exact M1X0 entry contexts now use precise native code:

- `$00:81BE`: the original queue wait/reset wrapper, including real calls to
  `$00:81F7` and `$00:81C7`. Its opcode comparison stops at the actual child
  entry with the real JSL frame intact; no child return is invented.
- `$00:899B`: the original DMA request/wait body. The source test covers both
  a still-pending request at a real deadline and a deterministic external
  `$023A` bit20 release on the fourth read, identically on both buses.

Each body passed 6,144 **new** original-opcode comparisons of full WRAM/IO,
registers/flags/hidden A, actual stack and CPU/master/coprocessor clocks. These
unit buses use constant eight-master accesses, so real device behavior was
checked separately. Exact body hashes bind those results to final generation.
The earlier 81F7 matrix, bridge tests and campaign were not repeated.

The final executable's same-EXE controls report 170 actual reset-wrapper entries
and one actual DMA-wait entry, versus zero when each target is interpreted.
Both comparisons preserve every ordered picture, checkpoint state/clock,
APU/DMA/PPU event, per-frame ARAM/DSP snapshot and captured sound sample.
A separate final-build/control four-door roundtrip covers Atland, Gold Cave,
Elf Field and Voloh: all 1,598 fields and six checkpoints match the timing58
build exactly. This comparison adds no timestamp exception.

`tools/qualify_wait57.py --target 0081BE` or `--target 00899B` selects the
corresponding strict live check. `tools/test_shared_services_oracle.py
--probe-service` accepts those same two exact targets. Existing 81F7 usage
remains available. The diagnostic counters are opt-in and do not alter guest
memory, input, interrupts, stack or admission guards.

The SDK patch and its qualified bridge are unchanged. Source/mode/stack/
generation checks and all other interpreter exclusions remain. In particular,
the M1X1 display setup at `$00:84E9` and the guarded `$01:814E` interrupt wait
have not been enabled by this batch. New inventory is 657 AOT/71 analyzed LLE
variants, not a completion percentage. Installed/released binaries stay separate.

Linux x64, Intel Mac and Apple Silicon Mac passed native compile/link of the new
bodies, hidden real GL draw/readback, POSIX paths/persistence and 20 package
guards in private run `41256092072`. The previously qualified unchanged SDK
bridge/capture suite was not repeated. This is not target gameplay, real-audio,
controller or campaign coverage; the DMA service has one positive live entry
on the measured route. Generated C and fixture states remain private.

## Timing60: queue drain and guarded interrupt-poll replay

The original `$00:8985:M1X0` DMA drain/wait now uses precise native code.
Its empty-queue branch, ordinary completion, original pending-bit40 polling and
multiple deadline positions passed 15,360 new ROM-opcode comparisons. Other
entry widths remain interpreted. A final same-executable device check records
one actual native entry versus zero in its interpreter control, with exact
pictures, complete state/clocks, APU/DMA/PPU events, ARAM/DSP snapshots and sound.

The `$01:814E` wait now uses the framework's existing guarded cycle replay when
eligible. The earlier short probe had no witnessed replay; a longer measured route
established activation without assuming the earlier non-activation cause. This is an interpreter wait
acceleration, not new AOT CPU coverage or a substitute for the original NMI.
The title checks the exact poll/jump, counter-reset and NMI-increment bytes
and original cartridge mapping every frame, including state loads/mod changes.
Changed source withdraws its declaration. The framework still requires stable
register/mode/stack state and read/write epochs, no volatile reads, no relevant
HDMA/coprocessor mutation, and no unmasked armed H/V IRQ. No guard was relaxed.

Same-executable off/on controls reduce actual interpreted poll opcodes from
1,817,467 to 10,333 on a 423-field assisted route. Every picture, checkpoint,
guest clock, device event, ARAM/DSP snapshot and captured audio sample matches.
Positive activation and fewer actual interpreted opcodes are required; an
unused declaration cannot pass. `tools/qualify_idle_poll.py` performs that gate.
`--idle-poll-control` approves only the same EXE/manifest and explicit0-to1
control input; all output comparisons stay strict. Its refusal/mixed-control
regressions pass, alongside all existing comparator cases (31 total).
The source admission test checks the original ROM plus all16 single-byte
mutations of the guarded windows and three null-pointer refusals.

The final default configuration also matches all1,598 fields and six checkpoints
of the four-door/state-load route against the retained timing59 control; that
finished control was reused. The earlier source matrices, SDK bridge suite and
campaign were not repeated. The unchanged SDK patch retains all prior guards.

A serial control/candidate/candidate/control measurement on one Windows PC,
without interpreter-counter callbacks or large trace captures, covers363
post-warmup frames per arm. Mean guest CPU work fell from1.068 to0.700ms (about
34%); per-arm95th-percentile costs fell from about1.86 to0.92ms. Median costs
were essentially unchanged. This measures the tested route's guest work, not
a universal FPS gain, physical presentation pacing or target-system performance.

The guarded replay is enabled by default; `LAGOON_IDLE_POLL_PROBE=0` is an
isolated developer control. Explicit0/1 plus `tools/qualify_idle_poll.py` enables
actual-opcode reporting. The display-entry widths remain unqualified. Current
inventory is658 AOT/71 analyzed LLE variants, not a completion percentage.
Existing installed game and public release binaries are unchanged.

Linux x64, Intel Mac and Apple Silicon Mac passed native compile/link of the
new body/title policy, strict-comparator tests, hidden GL draw/readback, POSIX
paths/persistence and 20 package guards in private run `41257655821`. The SDK
and its previously verified replay guard tests are unchanged. These hosted
checks are not target gameplay, real-audio or controller campaigns. ROMs and
seed states were not uploaded; generated C stays in the private build repo.

## Timing61: actor redraw preparation and bounded coverage

The original `$01:A4D6:M1X0` actor redraw preparation now uses precise native
code. Its clamp, flag reset, screen projection and visibility/sort tail passed
15,360 new opcode comparisons: 3,072 complete, 9,216 genuine deadline boundaries
and 3,072 entry deadlines. The fixture executes the actual emitted clamp and
projection children; their optional host shortcuts decline in the oracle only.
Production shortcuts and framework source/mode/stack/generation guards are
unchanged. Other entry widths and `$01:C726` remain interpreted.

A same-executable interpreter/native check witnessed four native entries versus
zero in its control, with exact pictures, state, clocks, device events, ARAM/DSP
snapshots and audio samples. Fresh timing60 controls also match the four-door
transition route and Evil Spirit/Zerah health-rendering fixtures exactly. Old
completed matrices, bridge suites and campaigns were not repeated. These are
affected assisted routes, not a full playthrough or a universal FPS claim.

`tools/qualify_native.py --coverage-summary` exports a final coverage snapshot
without retaining a raw journal. The strict comparator distinguishes summary,
journal and ordinary runs; all32 comparator and13 qualification-input tests pass.
The route snapshot is about314KB and reports no overflow, dropped cost samples
or journal failures. Its runtime ROM contains approved QoL patches, so it is
scouting evidence and must not be ingested as clean-ROM generator proof.
Unobserved M1X1 display entries remain excluded. Already guarded native windows
are not missing work merely because they appear in static LLE inventory.
Inventory659 AOT/71 analyzed LLE is not a completion rate. The interpreter stays
supported, and installed/released binaries are unchanged.

## Timing62: larger local batch; publication paused

This batch stays local at the owner's request. The installed0.2.5 release and
published platform binaries are unchanged; no hosted builds were requested.
Windows compilation and affected software/dummy-audio routes are the evidence
here, not physical-controller/listening/GPU tests or another campaign.

Twenty-six original room palette/tile/scroll handlers now use precise M1X0
native bodies. Their real upload child executes in399,360 opcode comparisons;
each handler also has a matched actual-map device replay with a positive
interpreted-entry count in its control and none at that entry in the candidate.
All pictures, checkpoints, clocks, device events, ARAM/DSP and audio samples
match. Other widths remain original. The damage hazard adds a source prefix at
01DD07 and continuation at01DD22:30,720 comparisons and a matched room26 route.
The actual nonlocal HP call at01DD1F remains original; no return mode is guessed.

The static scene path now separates original C643's nine-instruction prefix
from C654's pointer tail. Its native allowlist contains C726 only; other actual
table choices and unknown pointers retain the original interpreter and inherited
three-byte owner frame. The original C726 body passed15,360 new entry/deadline
cases before its real render/upload children; the dispatcher adds15,360 cases
with its actual C654 child and original unknown-target handoffs. A source-bound
same-executable replay witnesses a native dispatcher entry versus zero in its
control. The complete2,151-field new-game route also matches the previous
qualified executable exactly, including all5 checkpoints and captured audio.
A separate bounded snapshot observes C726 interpreted entries fall142 to0.
Bounce counters alone do not enumerate direct native calls. Early scene probes
with no actual entry were rejected; their green pixels were not qualification.

Cold boot's original twelve-opcode WRAM clear is now a guarded native source
window. It witnesses the actual8033..803A forced-blank/NMI-disable instructions,
requires the primary CPU/RAM/current generation, original mapping and bytes,
PB/DB/D0, E0/M1X0/I1, S01FF and no invented return frame.384 opcode/resume
comparisons include complete RAM/IO, registers, flags, open bus and all clocks;
84 source/mode/stack/generation refusals preserve guest state. A cold start
witnesses401,410 native source instructions, one completion and29 real field
deadlines. Loading a gameplay fixture mid-clear correctly retires its witness.

The SDK now exposes callback-scoped source-opcode clock commit and deadline
queries. They preserve original refresh, beam/copro sync, write epochs and both
relative boot and mapped absolute APU cadence. Legacy WRAM-progress flushes are
preserved too. Ordinary AOT ownership/deadline, IRQ, epoch and source guards are
unchanged. An initial prototype overshot a field because the AOT deadline query
correctly required a caller frame; another lost legacy progress-flush phase.
Both were rejected by fresh cold-state/device comparisons and fixed. Actual
compiled helper tests cover256 cadence/epoch cases,9 immutable refusals and
callback-scoped bounds. Final20/120-field cold and423-field load/gameplay checks
match the previous executable; affected four-door checks match1,598 fields and
six checkpoints. No failed prototype is installed or published.

The remaining original84E3 display wait uses the SDK's existing guarded cycle
replay. Every frame rechecks the real poll, request and NMI-decrement bytes and
mapping. The old1814E wait is preserved independently. A same-EXE explicit0/1
control observes5,744,605 versus72,610 actual interpreted poll opcodes on the
new-game route (about98.7% fewer), with identical pictures/state/clocks/events,
ARAM/DSP and audio. This is wait acceleration, not new AOT CPU coverage or a
universal FPS claim. Source admission rejects all17 single-byte mutations and
three null inputs. Developer control is LAGOON_FRAME_WAIT62=0; explicit1 enables
it. Diagnostic LAGOON_IDLE_POLL_PROBE=1 reports actual counts. The comparator
allows only this exact same-EXE/manifest0-to1 toggle and keeps every output
strict;35 comparator and13 qualification-input tests pass.

Current inventory690 AOT/68 analyzed LLE is not a completion percentage. Some
LLE names already contain qualified guarded scanner/iterator windows. Remaining
resource expansion, SPC-port and patched-camera contexts need their own source
and timing proof; asynchronous SPC-port polls must never be declared pure NMI
waits. Interpreter fallback remains supported. Original dirty SDK files, owner
saves/settings and the installed executable were preserved. Compact local
receipts and the next-work inventory live in validation/timing62; completed raw
captures are disposable through the reviewed owner-run cleanup command.


## Timing63: decoder, volatile upload acknowledgements and new-game clear

Local development batch only; publication and installation remain paused.
New source windows preserve actual source bytes, primary CPU, physical stack,
generation and callback deadlines. They leave the supported interpreter intact.
The existing menu/door expansion fast path and complete mapped sound uploader
retain their separate guards. No new generated AOT roots are claimed.

The decoder witnesses its real9143 entry, including native entries that later
yield into the original body. It executes dictionary clear, traversal, output
and real stack-data instructions with precise timing. Original return and input
boundaries remain. Cold/unmapped sound uploads keep real FF69 JSR/PHP ownership;
the three byte acknowledgment loops read every actual volatile APU reply using
the original callback bus and pending-clock synchronization. This is never an
idle/NMI replay. Source-proven direct native children also register their real
frame before yielding. The independent9304 RAM clear witnesses the original
frameless80AF jump and yields through27 field boundaries, without inventing a
caller from stale return-width metadata.

On the observed opening route,4,975,463 source instructions execute in these
native windows. This is an activation count, not a speed or completion claim.
31,392 original-opcode/deadline/resume cases and4,297 immutable guard refusals
pass. The callback bus helper has18 actual-helper read cases and10 authority
refusals. Matched opening/cold/load, item/magic/equipment and four-door routes
retain exact ordered pictures, checkpoint state/clocks/events and device/audio
outputs. Private receipts preserve the exact executable epochs per comparison.
No complete campaign or new target-platform qualification was performed.

An affected audio-enabled fast recipe also exposed a pre-existing diagnostic
repeatability limit: the old executable differs from itself when the SDL dummy
consumer drains an overflowing playback ring on wall time. The failed sample
comparison remains recorded; no audio waiver was added. Deterministic affected
controls disable the host playback device on both sides while continuing SPC,
DSP and generated-sample capture. This does not qualify physical listening.
Remaining observed contexts include payload/word-handshake continuations and
decoder input/raw-copy windows. Existing camera/source/width exclusions remain.


## Timing64: remaining decoder and sound-upload bodies

Local only; no publication or installation. Remaining decoder header, input,
dictionary-table, raw-copy and register cleanup instructions are statically
translated with source widths and real frame/generation/deadline ownership.
The upload continuation covers payload and byte/word handshakes through its
original PLP/RTS boundary, delegating the existing volatile word bus. On the
opening route,881,081 decoder and696,563 upload interpreted body instructions
become zero.38,820 new opcode/resume cases and24 new refusals pass. Raw-copy
content was not observed in the gameplay routes; its complete original
entry-to-RTL unit fixture is separate evidence. These are source windows,
not new generated roots or global native-completion percentages.

Callback word/byte bus helpers preserve original observation and synchronization
and source commit accounts for their real write epochs. A conservative8KiB
hook filter skips unrelated PCs; exact full-key lookup and512-slot registration
remain authoritative. Independent helper and registration/filter checks pass.

A diagnostic verification limit was found: unpaced undrained sample files can
stop at8192 pairs when the playback ring fills. Earlier pictures, full per-field
ARAM/DSP registers, event timing and CPU checks remain valid; their PCM must not
be called a full emitted stream. An explicitly guarded offline diagnostic sink
now consumes delivery after capture and verifies every span against the actual
producer counter; loss and flush failures invalidate the run. It requires host
playback disabled and changes no default gameplay setting. Same-executable
explicit64off/on controls match1,147,983 complete stereo pairs across2151opening
fields, and98,947 across actual decoder/upload state restores. Every observed
output remains strict. Executable epochs are recorded in private receipts.

The interpreter remains for real returns, unowned restored continuations and
other unsupported services. Existing native families and the owner's completed
campaign were not replayed merely to raise counts. Upstream issue233 is closed
by merged PR234; Lagoon already contains its core reset/load cursor correction.
A future SDK refresh must reconcile the overlapping patch and additional
upstream hardening while preserving the cumulative local SDK changes.
