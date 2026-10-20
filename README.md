# Lagoon Recomp

A work-in-progress Windows port of Lagoon (USA), using SNESRecomp's generated
native routines alongside its 65816 interpreter safety net. Your own clean ROM
is required. The executable does not depend on Mesen; it is a development reference.

## Current progress - October 20, 2026

Batch23 adds native Nasir death animation, guarded boss death-flash loops and
exact room08 Elf Field custom rendering: **387 AOT /119 analyzed LLE /268 roots**.
The exact tested build is installed; cold checks match 48 component hashes.
The interpreter safety net remains. Counts are not completion percentages;
the owner and friends perform the uninterrupted campaign playtest.

Nasir's original four-stage ordinary death animation and its paired selector
now run natively. Separately, boss/special-actor death flashes resume across six
original render/wait cycles with their real saved registers and caller frames.
Ten final old/new strict route pairs pass 38,274 ordered observations and 20
complete state/clock checkpoints, including all seven assisted boss wins,
revival and player death. The watched Nasir animation changes from 37 interpreter
entries to zero. Existing revival/refill arithmetic is preserved, rather than
newly rewritten. The five new generated bodies pass 57,600 original-opcode cases.
These are bounded, assisted checks, not complete native death/boss orchestration.

Room08 Elf Field joins stable Atland and exact Gold Cave03..07 in the custom
scene renderer. Three final same-executable off/auto routes match 3,229 ordered
observations and nineteen checkpoints, with 2,458 replacements and zero mismatches.
A separately source-proven correction fixes 99 expanded-picture observations
per route during Elf Field's opening quake. Those three old/new strict image
reports remain false; native pixels, complete checkpoint components and clocks
are exact. No CRC differences were hidden. The full reference renderer and
unsupported-field fallback remain; this is not new artwork or a GPU speedup.

See [batch23](NATIVE-BATCH23-2026-10-20.md),
[the custom renderer](docs/CUSTOM-RENDERER.md) and
[frame pacing evidence](docs/PERFORMANCE.md). The final hidden1080p real-audio
room07 sample measures 60.09 updates/s across 550 intervals, with p95 17.35 ms,
max 17.81 ms and no interval above 25 ms. This is not a locked60 or scanout claim.
Previous guarded refill and room07 work remains in
[batch22](NATIVE-BATCH22-2026-10-18.md).

The five audited menu checks open Items/Magic/Equipment in20/18/19 fields and
close in22 using twelve original charged DMA jobs. Giles accepts wound dressing
without a potion and follows through his original quest script. Optional improved
sword reach adds three lateral pixels on each side to its existing38px reach.

Guarded host computation of two original enemy-position leaves improves measured
Gold Cave gameplay cadence without skipping AI, collision or hardware work.
Batch19's1920×1080 real-audio tests measured about60.12 presentation FPS; the sampled
cave changes from69 three-field updates to525 two-field updates with no slower
updates. This is bounded evidence, not a locked60FPS or all-room guarantee.
Room02's44 incoming water-fade fields and room03's46 outgoing fade fields now
retain independently source-verified scenery and native center pixels.

Focused checks cover actual damage, seven boss death/defeat endpoints, original
rewards, consumable revival and quest interactions. Broader native death/boss
orchestration, unsupported scenes and hardware/controller coverage remain open.
Read the [playtest guide](docs/PLAYTEST.md), [batch19 stabilization evidence](NATIVE-BATCH19-2026-10-17.md),
[current status](docs/DEVELOPMENT-STATUS.md), [todo list](TODO.md),
[menu details](docs/FAST-MENUS.md), [widescreen scope](docs/WIDESCREEN.md),
[performance evidence](docs/PERFORMANCE.md) and [save states](docs/SAVE-STATES.md).
Earlier results remain in [batch18](NATIVE-BATCH18-2026-10-17.md); historical
exploration is not relabeled as a natural complete playthrough.

## Fullscreen screenshots

[View the 1,920×1080 fullscreen captures](docs/screenshots/README.md), including
wide town dialogue, Items/Magic, Atland and Gold Cave.

![Atland dialogue in widescreen fullscreen](docs/screenshots/atland-dialogue-21x9-1,920x1080.png)

## Launcher

Use `Start Lagoon.cmd` after building. It opens the shared
[Companion Launcher](https://github.com/rages4calm/companion-launcher), using pinned
0.2.0 packages. The Lagoon adapter verifies your ROM and starts the native game
with its existing config and save locations. The old pre-boot UI is excluded from
the default build; it can be enabled explicitly for development.

Companion exposes executable/ROM/config paths, native display/audio/gamepad
settings, gameplay QoL toggles and experimental widescreen aspects. Apply/Revert
preserves unknown settings and controller bindings and detects stale edits.
Existing native settings remain in `build/config.ini`, input in `build/keybinds.ini`,
and saves in `build/saves`. The adapter does not migrate them.

For a 1,920×1080 monitor, use desktop fullscreen, widescreen 16:9 and optional
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
outdoor guard exterior (room 2), audited cave rooms 4–6, room 07 parallax and
Elf Field (room 08), clipped to real world bounds. Verified dialogue/Start retain scenery and Items/Magic widen their
frames. Other rooms, title/intro and unadapted panels use native-width fallback.
Special actors, other parallax layouts and campaign coverage remain unfinished. All four fixed aspects preserved guest state and native
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
