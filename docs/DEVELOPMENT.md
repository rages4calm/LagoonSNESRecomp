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

## Deferred after v0.3.0

The next map/UI pass should investigate a full-zone map resembling the actual
area, clear doors/exits and useful landmarks, plus readable map and special-menu
text styled consistently with Lagoon. Review the user's video around 28 seconds
when doing that work. This is deferred until release completion; no map redesign
or new artwork is included now. Broader objective guidance and optional zoom-out
remain later considerations.
