# Lagoon Recomp

A work-in-progress Windows port of Lagoon (USA), using SNESRecomp's generated
native routines alongside its 65816 interpreter safety net. Your own clean ROM
is required. The executable does not depend on Mesen; it is a development reference.

## Current progress — October 17, 2026

This is a playable development port with generated native routines and an
interpreter safety net. It is **not a fully native or fully qualified campaign**.
The selected program still has **190 AOT variants, 18 declared LLE variants and
47 promoted gameplay roots**. Native work includes movement, combat helpers,
damage arithmetic, EXP/gold/level progression, first-boss logic and bounded
HP/death HUD tails. Remaining death, boss and dispatch paths retain fallback.
Counts are not completion percentages.

The new accelerated campaign checks use explicit teleports, player stat cheats
and some progression setup, then run original room loading, combat and outcomes:

- **73 of 75 room records** reached through genuine door loading. The cloud
  script-only entry and unused/alias record remain separate coverage gaps.
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

A private coverage-generated program with 348 AOT variants was built and probed.
It remains **unselected**: strict state/clock comparisons differ, and one Ella
presentation differs from the control. The tested 190-variant program remains
the playable build. See [current status](docs/DEVELOPMENT-STATUS.md),
[boss checks](tools/CAMPAIGN-BOSSES.md), [endgame checks](tools/CAMPAIGN-ENDGAME.md)
and [coverage workflow](tools/CAMPAIGN-COVERAGE.md).

Widescreen supports 16:9, 16:10, 21:9, 32:9 and window-driven aspects in audited
areas. Supported town dialogue, Items/Magic, outdoor scenery and Gold Cave rooms
have guarded expansion. Outgoing fades retain proven old scenery until the real
PPU changes resources. Unsupported areas and effects retain reference output;
remaining menu, scripted-scene and transition gaps are still being tracked.

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
[save-state details](docs/SAVE-STATES.md). The optional Faster menus toggle trims
Items/Magic fade delays while preserving resource loading.

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
