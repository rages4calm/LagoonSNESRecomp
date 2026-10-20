# Batch26: centered-camera native dispatch

Centered-camera gameplay now reaches the existing native actor and boss dispatcher
through a guarded handoff. The interpreter safety net remains enabled. This batch
adds no generated bodies: the manifest remains **408 AOT /112 analyzed LLE /287
roots**, which are analysis counts, not completion percentages.

## Cause and implementation

The approved camera patch changes the main call at `01:80CA` from `JSR A7B5` to
`JSR 83D8`. Its camera trampoline runs the original patched camera/strip work,
then uses `JMP` at `01:83F3` to reach `01:A7B5`. The interpreter previously entered
native functions through real calls, so this tail jump bypassed already compiled
actor and boss work. A focused old-build control confirmed that camera-on retained
648 interpreted A7B5 entries, including 72 ACAA and 72 Thimale AEB9 entries; the
same camera-off configuration had none. These different settings establish the
cause; they are not a state-parity comparison between camera options.

`src/lagoon_native_camera.c` now witnesses the **actual** `80CA → 83D8` call. An
optional SDK hook observes only the actual registered `83F3 → A7B5` jump and
validates the existing two-byte caller frame. The native return remains the real
`01:80CD` owner. It does not push a replacement frame or invent an exit mode.
The existing paired native-call path handles deadlines, yields, nonlocal unwinds,
SKIP and APU state. Generic jump promotion remains disabled; an unset or refused
hook leaves interpretation in place.

Acceptance requires the same CPU, RAM, stack pointer and runtime generation,
actual `CC 80` return bytes, M1/X0/E0, PB1/DB1, direct page zero, binary arithmetic,
and consistent packed width/decimal flags. The linear two-byte WRAM frame uses
the existing bounded `S <= 1FFD` contract. Source hashes cover the approved main
code (83 bytes at `01807A`), complete patched camera/strip helpers and tables
(1,259 bytes at `018218`), and original actor dispatcher/table (33 bytes at
`01A7B5`). The witness is consumed once. Existing recovery reporting includes
actual validated `camera_tail` takes; these count handoffs, not native instructions.
All 22 existing wait-continuation descriptors remain unchanged.

## Verification and executable identities

The title-token and actual SDK matcher tests pass **5,599 checks**, including
every guarded source-byte mutation, wrong owner frames, stale generations,
decimal/width contradictions, replay and refused/unset hooks. The camera opcode
oracle passes **102,144 cases**: 14,288 complete returns, 36,784 original child
boundaries and 51,072 deadline-before-native-entry yields. It executes the original
patched camera instructions and unchanged generated actor/boss bodies, comparing
full RAM, registers, flags, saved/dead stack and CPU/master/coprocessor clocks.
Unsupported children stop at their actual original entry; they receive no fake
return or outcome. This unit oracle uses a constant-cycle test bus; live pairs
separately check real scheduler/APU behavior. Existing recovery/continuation tests
also pass.

The first native candidate was
`5d875412a89343a497d6d649b10e6babfa34224f7504548a696701a33967c9c7`.
Two strict old/new short pairs, each retaining the same camera option and diagnostic
settings, matched **352 observations /4 checkpoints**. Camera-on recorded 648
actual handoffs; camera-off recorded zero. The relevant interpreted dispatcher
entries disappeared in the enabled candidate.

The final coherent executable is
`91d8c0119e304e2cc5424d960f00629d0e475916aaedd203f1ba680fce6effb1`.
Independent source/object comparison binds the unchanged native camera, recovery,
bridge and generated components from the first candidate to this executable.
Renderer components changed in between and have separate qualification in
[CUSTOM-RENDERER.md](CUSTOM-RENDERER.md).

Fresh prior-build controls and final-executable camera-enabled pairs passed:

| Assisted route | Ordered observations | Eight-component checkpoints | Actual handoffs |
| --- | ---: | ---: | ---: |
| Thimale finish | 6,965 | 2 | 9,768 |
| Ella player death | 1,521 | 2 | 1,158 |
| Red Spirit entry/ending-handler route | 1,803 | 2 | 1,108 |
| Consumable revival | 3,666 | 2 | 11,714 |
| **Total** | **13,955** | **8** | **23,748** |

All compared guest state, clocks and presentations matched, with zero handoff
refusals on these routes. The unchanged generated manifest SHA256 is
`63abf343c3e27f2a008a352df7dd64b4121962fcdd1f0b2dd1ffac44d4d4d598`.

## Reproduction and remaining work

From the repository, with Clang, Python and your own pinned clean ROM available:

```powershell
python -P tools/test_camera_tail.py --rom "<owned ROM path>" --out "<fresh guard output>"
python -P tools/test_camera_tail_oracle.py --rom "<owned ROM path>" --gen src/gen --out "<fresh oracle output>"
pwsh -File tools/test_native_recovery.ps1 -Rom "<owned ROM path>" -OutDir "<fresh recovery output>"
```

The opcode test requires locally generated code matching the manifest and the
project's cumulative SDK changes. `-P` prevents local tool names from shadowing
Python standard-library modules. Assisted runtime pairs also require local fixture
states and matching settings; those inputs, ROMs and raw captures are not public.

The original camera/strip instructions still run interpreted before the handoff.
Unsupported source, mode, frame, generation or child-service contexts retain
their existing fallback. The Red route reaches the ending handler; it does not
qualify the full credits sequence, and the B90B ending loop remains interpreted.
This verifies affected boss/death/revival routes, not a fully native game or an
uninterrupted campaign. The owner and friends retain the full playtest.
