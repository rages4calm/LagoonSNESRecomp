# Development

## Native CPU architecture

The v0.3.0 release line runs 100% of main 65816 game CPU execution natively.
The original interpreter remains an explicit debugging/reference control;
normal release execution does not fall back to it. An uncovered or mismatched
instruction stops before execution. PPU rendering, DMA, clocks and SPC700/DSP
audio remain hardware-runtime responsibilities. Native execution and a matching
handwritten C decompilation are different deliverables.

Lagoon uses the pinned SNESRecomp framework and cumulative
`patches/lagoon-framework.patch`. Existing compiled/source-qualified services
retain their fast paths. A generated architectural continuation backend supplies
source-specific PC/M/X/E entries with real calls, returns and interrupt frames,
so yielding or restoring a continuation does not require runtime opcode decoding.
Two admitted code profiles cover retail and the exact centered-camera patch;
other supported QoL selections use data or guarded read/write views.

`config/native69/` contains reviewed addresses/modes, original ROM identity,
compiler pins and patch admission, not ROM bytes or generated game C. Generation
checks source identity and the existing camera writer before emitting private C.
Runtime attestation, physical mapping, mode/stack and generation checks remain
independent. Never replace a refusal with an empty function, fabricated quest
result or synthetic return.

Source closure accounts for known function and dispatcher targets. Static
candidates in unproven modes are not assumed live merely because they decode.
Unknown overlays, edited return frames and out-of-domain targets remain refused;
arbitrary modified-ROM/state/mod support is not implied by the native release.

## Build and reference mode

[Build instructions](BUILDING.md) · [Native profile inputs](../config/native69/README.md).
Playable builds require both native profiles and strict defaults through
`LAGOON_REQUIRE_NATIVE_CPU=ON` and `LAGOON_NATIVE_RELEASE_DEFAULT=ON`.
Preserve the pinned framework/patch and existing dirty checkouts. Changed compiler
or ABI inputs need review and affected-route qualification; updating hashes to
silence a refusal is not qualification.

To select the reference interpreter deliberately, set both
`LAGOON_NATIVE_BACKEND69=0` and `SNESRECOMP_NATIVE_ONLY=0`. Normal releases leave
these overrides unset. A non-native development configuration also needs both
native CMake requirements explicitly disabled. Do not publish a reference or
setup-host executable as the playable game.

## Verification

CPU qualification includes 95 exact native/reference cases, 167,278 ordered
presentations and 164 retained hardware checkpoints. They execute 567,962,367
generated native instructions with zero general CPU interpreter invocations or
unqualified guest replay. Source closure and dynamic-pointer contracts supplement
finite routes; counts alone are not a percentage of the game. All 74 nonempty
original rooms have resource-entry checks, distinct from every transaction,
quest branch or movement sequence. Menu/save/recovery evidence is separate.

The final ending route checks original Bird defeat, pendant interaction,
final-Red defeat and the book. The ending achievement accepts separately
source-qualified direct and dispatcher caller frames, retaining actual stack
ancestry, source, generation and terminal-state requirements. A route name or
sparse screenshot is not an ending witness.

Recent rendering checks replay the owner's F1/F2 inputs. BG1/BG2 camera phases
are sampled independently, preventing whole margins from blanking during vertical
movement. Transition presentation retains the outgoing fade's low-water level,
so an incomplete loader cannot brighten and resurrect the old doorway after
blanking. Positive incoming source authority retires the outgoing canvas across
later refusals. Before/after checks preserve guest state and, where only margins
change, every original center pixel.

Source unit tests, synthetic coordinate/policy fixtures, cheat-assisted runs,
hidden GPU captures, installed-artifact checks and natural playthroughs prove
different things. The owner completed the Windows campaign; do not repeat it to
increase native counts. Hosted platform compile/GL/persistence/UI checks are
not full target campaigns or real-device audio/controller tests.

Matched comparisons need identical ROM, initial state, input timing and execution
points. Diagnose the first register, memory, I/O or clock divergence. Preserve
failed receipts and unexplained differences; do not normalize arbitrary PCM or
clock differences into a pass. Debug acceleration must preserve actual call
frames, original event consequences and hardware timing.

Public focused checks include:

- `tools/test_wide_transition_protocol.py`: source-extracted menu/readiness policy,
  temporal transition cache and per-layer raster coordinates.
- `tools/test_achievement_ending.py`: guarded ending observer and actual SDK
  caller-prefix checks with an owner-supplied ROM.
- `tests/test_generate_native_backend69.py`: metadata, compiler pin and LF/CRLF
  admission checks without private assets.
- `tests/test_run_native_backend69.py`: strict launch arguments/environment and
  child-exit propagation without opening the game.

The [timing contracts](../tools/NATIVE-TIMING.md),
[analysis policy](../recomp/README.md) and
[bounded warps](../tools/CAMPAIGN-WARPS.md) retain useful reference details.
Interpreter descriptions in old reports describe those earlier controls,
not the normal v0.3.0 execution policy.

## Rendering and pacing

The title compositor expands qualified scenery/actors while preserving source
identity, original center pixels, layer order and finite map extents. The custom
renderer and configured GPU presentation do not replace every PPU layout,
effect or original pixel asset. Unsupported visual content can retain its
original view without invoking the CPU interpreter.

Measure guest work, raster/scene composition, GPU upload/draw/swap, diagnostic
I/O, limiter waits and audio backlog separately. A steady display counter does
not guarantee constant game-logic speed. Hidden/dummy-device checks do not
certify scanout or listening. There is no full-game locked-60-FPS claim.

## Contributions and private inputs

Keep dependency notices, source/mode/stack/generation guards and the framework
patch. Do not reset the SDK or stage a dirty submodule link accidentally. ROMs,
ROM-derived C, decomp exports, saved games, extracted Lagoon assets and raw
validation remain private. The credited cameo retains its original copyright;
software MIT licensing does not relicense it. The owner-approved converted
X68000 album is the explicit audio distribution exception.

Public source contains reusable code, build inputs and important guides. Keep
per-run handoffs and long chronological lab reports local; Git preserves source
history. Publish substantial verified batches with accurate artifact checks.

## 0.4.0 map and navigation features — final checks in progress

The new full-zone map renders terrain from the latest immutable, qualified area
source: both map layers, metatile definitions, tile patterns and palette. It does
not stitch screenshots or advance gameplay to uncover art. A finite area can be
up to 2048×2048 world pixels; the cached image uses original resolution or an
integer twofold reduction so neither side exceeds 1024 pixels. Actors, the HUD
and transient effects are excluded.

Scenery resource contracts cover original area IDs 00–4A. Parallax handling is
explicit for 07, 0C, 12, 13, 28, 29 and 2A: Gold Cave, Philips Castle, Dwarf Cave,
Phantom Hill and the Cloud. Full BG1 geometry is combined with the original BG2
page layout and priority at the qualified scroll anchor. This is not an animated
world simulation or a claim that every room has had a new live playthrough.

Room, resource and state-generation identity must match the completed field.
A presentation-only refusal may reuse current-room terrain only after fresh exact
terrain/resource equality. Another room, generation, loading field, fade or changed
art cannot reuse it. These live-world guards are independent of the UI archive.

Only a committed, non-speculative host field can add its qualified terrain source
to the visited-area archive. Browsing uses a separate decoder/image buffer and
does not grant world-render authority. Up to 75 bounded terrain-only records use
about 5.3 MiB; explicit little-endian serialization binds clean ROM identity,
resource descriptors/definitions, dimensions and CRCs. Loading merges older
records without replacing fresher session data. Disk I/O occurs only at map
opening/closing or quit. Corrupt, foreign or concurrently changed files are
preserved. The local `lagoon-scenery.maps` file is never a release input.
Missing archived scenery retains the labeled explored layout; older exploration
files gain scenery on a fresh qualified visit.

The 960×540 map supports pan, four zoom levels, fit and recenter controls with
fixed-size labels. Full scenery visibility is independent of persisted discovery.
Numbers associate discovered exits with their paged destination list; an unknown
destination remains unnamed. Chest visibility/opened state, early tablet guidance
and exploration persistence retain their existing authority. Opening, rendering
and navigating the map do not write guest progress.

Options and achievements use the shared independent font/frame renderer in
`src/lagoon_ui.c`. The licensed font generator and provenance are in
`assets/fonts/atkinson-hyperlegible/`; playable packages include the OFL and
provenance. Readable host text is composed separately from the gameplay EPX pass.
Compact achievement-toast dimensions, timing and progress behavior are retained.
[Design record](../DESIGN.md) · [Player controls](PLAYING.md).

Options has five tabs. Sound preserves its three existing actions. Journal uses
bounded read-only story/item views, and Items has 25 original-name/effect entries
with actual Atland/Voloh prices. Display persists `ShowSwordRange`, default off;
the attack cue is a presentation of the active reach and cleave, not an added hit.

Travel is a separate guarded action, not a generic warp. It supports the original
main entrances of Gold Cave, Philips Castle, Dwarf Cave, Siegfried Castle, Ice Cave
and Lagoon Castle after their original boss-victory flag. It requires a stationary,
safe ordinary scene, no nearby threats or active escort, and a valid entrance and
room path. Boss arenas, unsupported host modes and incomplete transitions refuse.
The panel requires two activations and emits a one-shot request; the live adapter
revalidates before the game resumes. The original door scanner and room loader
perform the transition using the authenticated original entrance payload. It does
not manufacture a CPU call/return, directly assign the room or grant quest flags.

Source/model fixtures, input/persistence tests, GPU readback and live affected-route
checks have separate scopes. Live map checks cover Atland, the Gold Cave entrance,
an interior and its parallax area, including exact 1080p UI readback and matched
post-menu state. The other parallax areas have source/geometry fixture coverage,
not a new live GPU claim. Loaded-save returns from Gold Cave's entrance area and
a deeper room reached the original entrance with health, inventory and story
flags unchanged and zero CPU interpretation. All six entrance groups have
source-backed model tests; this is not an all-dungeon live campaign or new target-OS
playthrough is implied. Preserve refused/failing cases until explained rather than
counting a synthetic model pass as an executed native room load.

The [Triforge map reference](https://www.youtube.com/watch?v=0IaGDQ_R-YE&t=32s)
informed map navigation; Lagoon uses its own original terrain and a flat map.
Gameplay zoom-out and broader optional quest guidance remain later considerations.
