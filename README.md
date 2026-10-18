# Lagoon Recomp

A work-in-progress Windows port of Lagoon (USA), using SNESRecomp's generated
native routines alongside its 65816 interpreter safety net. Your own clean ROM
is required. The executable does not depend on Mesen; it is a development reference.

## Current progress — October 17, 2026

This is a playable development port with generated native routines and an
interpreter safety net. It is **not a fully native or fully qualified campaign**.
Selected batch 15 has **361 AOT variants, 123 declared LLE variants and 244 analysis
roots**, preserving all 348 previous native variants and adding 13 original boss
animation/state modes. The interpreter safety net and prior exclusions remain.
Native work includes movement, room/actor loading, actor updates, combat helpers,
damage arithmetic, EXP/gold/level progression, first-boss logic and bounded
HP/death HUD tails. Remaining death, boss and dispatch paths retain fallback.
Counts are not completion percentages.

The new accelerated campaign checks use explicit teleports, player stat cheats
and some progression setup, then run original room loading, combat and outcomes:

- **73 of 75 room records** reached through genuine door loading. Records 2A and 3F
  have no ordinary incoming door and remain separate classification/entry gaps.
- **Seven main bosses** defeated with original rewards and completion flags;
  Nasir also dies to each in separate fixtures.
- **Six endgame combat phases** exercise the shell, Green Spirit, Zerah,
  Thor, bird transformation and Red Spirit through original death/creation code.
  The ending handler is reached; the full credits sequence is not qualified.
- All six Thimale guardians die through actual sword hits and unlock the boss.
  Stock-ROM coverage proves six executions of the unusual nonlocal death escape.

These are assisted encounter checks, not an uninterrupted natural playthrough.
No accepted victory is inferred merely from a missing actor. Frozen private
receipts distinguish cheats from real HP changes, rewards and progression.
Canonical coverage is ingested through the SDK with identity, journal-loss and
runtime-failure checks; feature-patched ROM captures remain separate.

The earlier Ella rendering and native handoff timing regressions remain fixed.
Native work includes the original guardian-death escape, revival/recovery helpers,
HP/MP caps, hit selectors and witnessed recovery after cooperative waits. Batch 15
adds source-closed boss animation counters, staged frame selection and child
sprite-state helpers. Final matched-host combat/primary checks pass 26/26: 25 fixtures execute new
work, removing 186,682 interpreted instruction executions, 146,299 excluding returns.
The new bodies contain 174 unique original PC/mode keys; executions are counted
exclusively within each replay, not globally unique addresses or measured FPS.
Final 99 comparisons and actual 79-capture SDK ingestion pass.

All 25 timing/audio deferrals, camera seams and the 0080E6 opening interpreter floor
remain. The selected 361 program passes 99 matched comparisons against 348 under the same
updated host: 151,417 ordered presentations and 2,657 full checkpoints, including
SRAM, registers and guest clocks/beam. Room 01 native parity uses its explicit matched narrow configuration;
wide output has independent source and pixel checks. Public analysis roots and
timing selectors reproduce the program without private captures.
See [batch 15](NATIVE-BATCH15-2026-10-17.md),[timing details](tools/NATIVE-TIMING.md),
[current status](docs/DEVELOPMENT-STATUS.md),[boss checks](tools/CAMPAIGN-BOSSES.md),
[endgame checks](tools/CAMPAIGN-ENDGAME.md) and [coverage workflow](tools/CAMPAIGN-COVERAGE.md).

Widescreen supports 16:9, 16:10, 21:9, 32:9 and window-driven aspects in audited
areas. Supported town dialogue, Items/Magic/Equipment, room-01 interior scenery, outdoor scenery and Gold Cave rooms
have guarded expansion. Batch 15 also expands Elf Field (room 08)
with independently verified background and actor source pixels. Outgoing fades retain proven old scenery until the real
PPU changes resources. Unsupported areas and effects retain reference output;
Room 01 status and its audited destination fade retain wide scenery through
verified torch animations. Equipment now extends its original frame, chain and
divider in room 01, Atland and the cave while preserving original title/body pixels. Remaining menu, scripted-scene and transition gaps
are still being tracked. The [area-map references](docs/MAP-REFERENCES.md) help
plan coverage routes.

The [live custom renderer](docs/CUSTOM-RENDERER.md) independently draws supported
stable Atland fields from copied maps, tiles, palettes and actors. Every output
pixel must match the reference before replacement. The reference PPU still runs;
caves and other unsupported fields use it. Visual similarity is expected.
This is original artwork, not a full retexture or a D3D12 backend.

The last normally paced hidden 32:9/Enhanced measurement averaged **60.10 FPS**,
with occasional spikes. Composition fell from 1.473 ms to 0.356 ms after caching;
this does not prove locked 60 FPS, a rendering speedup over reference alone, or
physical-display cadence. See [performance evidence](docs/PERFORMANCE.md).

Smooth/bilinear scaling was removed at the user's request. Enhanced original
pixels remains optional. Fresh version-3 states preserve live execution, PPU
ports and both raster journals; old/incomplete states refuse cleanly. See
[save-state details](docs/SAVE-STATES.md). Menu acceleration is now built in: audited Items/Magic/Equipment openings take
40/38/39 fields instead of 148/146/147. Batch 15 closes improve room 01 from 85 to 60
fields and Atland from 92 to 61, retaining 31/32 blank fields respectively. Original
queue-full waits still execute. Remaining loader work means transitions are not
instant. The original upload path and
unsupported decoder contexts remain intact. See [menu details](docs/FAST-MENUS.md).

The original opening-dispatch interpreter repair remains in place. Existing
native-versus-pure-interpreter timing differences and broad campaign/hardware
qualification remain open. See the [todo list](TODO.md).

## Fullscreen screenshots

[View the 1920×1080 fullscreen captures](docs/screenshots/README.md), including
wide town dialogue, Items/Magic, Atland and Gold Cave.

![Atland dialogue in widescreen fullscreen](docs/screenshots/atland-dialogue-21x9-1920x1080.png)

## Launcher

Use `Start Lagoon.cmd` after building. It opens the shared
[Companion Launcher](https://github.com/rages4calm/companion-launcher), using pinned
0.2.0 packages. The Lagoon adapter verifies your ROM and starts the native game
with its existing config and save locations. The old pre-boot UI is excluded from
the default build; it can be enabled explicitly for development.

Companion exposes executable/ROM/config paths, native display/audio/gamepad
settings, all ten QoL toggles and experimental widescreen aspects. Apply/Revert
preserves unknown settings and controller bindings and detects stale edits.
Existing native settings remain in `build/config.ini`, input in `build/keybinds.ini`,
and saves in `build/saves`. The adapter does not migrate them.

For a 1920×1080 monitor, use desktop fullscreen, widescreen 16:9 and optional
Enhanced original pixels. Keep linear filtering and frame blending off for a
sharp picture, with VSync on. Window scale controls only windowed size; leaving
it at 3 is fine in fullscreen. Wider 21:9/32:9 modes reveal more scenery but
make the picture shorter on a 16:9 monitor. The underlying 4:3 display-aspect
setting preserves original pixel proportions while widescreen adds scenery.

## Build from source

Install the Windows toolchain/dependencies documented by
[SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp): Python 3.10+,
Rust, CMake, Ninja, a supported C/C++ toolchain, SDL3 and zlib. Use a configured
build shell. Generate locally from your own original USA ROM:

```sh
git submodule update --init --recursive
python -P tools/apply_framework_patch.py
bash tools/regen.sh --rom "/path/to/Lagoon (USA).sfc"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On Windows, use Git's Bash rather than an unconfigured WSL stub. `regen.sh` also
applies/checks the required framework patch. `framework_pins.txt` records the
upstream baseline; `patches/lagoon-framework.patch` carries the local compatibility
and precise-root timing changes. The public submodule pin is reachable upstream.

Build CompanionLauncher 0.2.0's local NuGet packages, then:

```powershell
powershell -File scripts/build_launcher.ps1 -CompanionRoot C:\path\to\CompanionLauncher
```

The game expects SHA256
`46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb`,
CRC32 `d2554270`, a 1 MiB unheadered USA LoROM. Patched ROMs are deliberately
rejected by this clean-ROM build. Generated C is ROM-derived and remains local.

## QoL and widescreen

All nine requested QoL changes are implemented and default off. They use guarded
ROM data changes and shared CPU read/write hooks, so generated native routines and
the interpreter see the same gameplay values. Camera centering changes guarded
operands in explicitly interpreted routines, preserving the original scenery
streamer. The supplied BPS informed this
work; it is not bundled or automatically applied. See
[QoL implementation and limits](QOL-IMPLEMENTATION.md) for exact balance values
and the distinction between synthetic tests and gameplay evidence.

Optional widescreen draws additional world backgrounds in supported rooms at
the original pixel scale, with 16:9, 16:10, 21:9, 32:9 and window-driven aspects.
Witnessed ordinary NPC/enemy forms can also appear beyond the game's original
off-screen actor buffer, using resident graphics and read-only presentation
animation. Current scenery support covers audited Atland, Gold Cave and the
outdoor guard exterior (room 2), plus audited cave rooms 4–6, clipped to real
world bounds. Verified dialogue/Start retain scenery and Items/Magic widen their
frames. Other rooms, title/intro and unadapted panels use native-width fallback.
Special actors, parallax and campaign coverage remain unfinished. All four fixed aspects preserved guest state and native
center pixels at 18 opening/town checkpoints. See [widescreen details](docs/WIDESCREEN.md).

## Optional enhanced original pixels

Companion's Display page offers **Enhanced original pixels (EPX)**. It rounds
pixel diagonals using the original colors and keeps the bottom HUD sharp.
This follows the original-art treatment used in Secret of the Stars; the full
retexture experiment is shelved. EPX defaults off, requires OpenGL, and restores
your previous graphics settings when disabled. See [the implementation and
bounded GPU checks](docs/ENHANCED-PIXELS.md).

## Optional X68000 soundtrack

The optional soundtrack uses a privately imported VGZ pack, with loop points
preserved. Initial replacement covers opening, Atland and GoldCave; other cues
retain SNES music. SNES audio remains the default. No music is bundled in the
repository. See [setup and current limits](docs/X68000-SOUNDTRACK.md).

## Source and publishing

See [credits](CREDITS.md), including the original
[Lagoon QoL hack](https://www.romhacking.net/hacks/9856/) that inspired the
requested comfort changes.

This repository contains source, build configuration and development tools. It
excludes ROMs, generated ROM-derived C, private decompilation exports, extracted
assets, saves, states, screenshots and test captures. No playable release is being
published with this source snapshot. GitHub updates are deliberate batch snapshots,
not an automatic per-feature sync.

Retain the included licenses: the SNESRecomp foundation and its derivatives use
PolyForm Noncommercial; Companion Launcher and the Lagoon launcher adapter use MIT.
Public source availability does not remove the framework's noncommercial terms.
