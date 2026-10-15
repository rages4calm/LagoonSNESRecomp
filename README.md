# Lagoon Recomp

A work-in-progress Windows port of Lagoon (USA), using SNESRecomp's generated
native routines alongside its 65816 interpreter safety net. Your own clean ROM
is required. The executable does not depend on Mesen; it is a development reference.

## Current progress — October 15, 2026

Thirty-two gameplay roots have been promoted in the October 15 batches.
The first eight cover:

- Entity movement integration (`01:A4EB`).
- Room-boundary clamping (`01:A56A`).
- World-to-screen projection and visibility (`01:A5AA`).
- Direction/speed-tier velocity selection (`02:80EA`).
- Sprite graphics staging (`01:C579`).
- Knockback state updates (`01:C5ED`).
- Map/attribute quadrant sampling (`01:8385`).
- Jump eligibility/input checks (`02:8199`).

Thirteen more cover hit eligibility/rectangles, enemy defeat marking, gold and
EXP awards, level/stat recalculation and regeneration. Real bat combat,
level-up, gold saturation, temporary stat bonuses and equipment fixtures match
the previous native control. These fixtures explicitly seed a few setup values;
natural story progression and the full campaign still need qualification.

They use opt-in instruction-boundary timing and real bus-region costs. The
7,219-frame movement/menu route matches the prior native control at all 18 dump
tags, including framebuffer, WRAM, registers, VRAM, OAM, CGRAM and guest clock.
Interpreted work falls from the first batch's 70,655,953 to 63,863,965
instructions on that route, another 9.61% reduction. This measures interpreted
work on a bounded replay, not a native-completion percentage.

Six more cover player hit-direction dispatch/handlers and boss sprite offsets,
palette selection and jump-anchor projection. Real player damage/death and a
first-boss sword hit match the previous build at 924 checkpoints and all 15,123
presentations. Boss HP falls from 96 to 94 in a fixture using the sword-reach
option and explicit player positioning/equipment setup. A later bounded fixture now covers actual boss defeat and rewards; natural
post-boss story progression and complete damage/death orchestration remain open.

Five further roots cover player damage calculation, boss contact, death-stop
velocity, room hazards and the first boss quake. Four targeted routes match the
previous build at 953 checkpoints and 15,939 complete presentations, including
real damage, consumable revival and boss defeat/rewards. HP/death wrappers remain
interpreted; native callers dispatch using the actual returned CPU widths.

The build generates 175 AOT variants and 18 declared LLE variants; additional
runtime fallback remains available. The original opening-loop fix stays in place.
Existing native-versus-pure-interpreter clock/state differences remain unresolved.
Bounded combat/progression evidence does not certify every encounter or save.
See [the current evidence receipt](NATIVE-BATCH5-2026-10-15.md) and [todo list](TODO.md).

## Fullscreen screenshots

[View three 1920×1080 fullscreen captures](docs/screenshots/README.md), showing
Atland at 21:9 and 32:9 and Gold Cave at 16:9 with camera centering enabled.

![Atland widescreen fullscreen capture](docs/screenshots/town-21x9-fullscreen.png)

## Launcher

Use `Start Lagoon.cmd` after building. It opens the shared
[Companion Launcher](https://github.com/rages4calm/companion-launcher), using pinned
0.2.0 packages. The Lagoon adapter verifies your ROM and starts the native game
with its existing config and save locations. The old pre-boot UI is excluded from
the default build; it can be enabled explicitly for development.

Companion exposes executable/ROM/config paths, native display/audio/gamepad
settings, all nine QoL toggles and experimental widescreen aspects. Apply/Revert
preserves unknown settings and controller bindings and detects stale edits.
Existing native settings remain in `build/config.ini`, input in `build/keybinds.ini`,
and saves in `build/saves`. The adapter does not migrate them.

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
outdoor guard exterior (room 2), clipped to its actual world bounds. Other rooms, titles, intro and menus keep clean native-width layouts.
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
