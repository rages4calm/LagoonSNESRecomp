# Lagoon Recomp

A work-in-progress Windows port of Lagoon (USA), using SNESRecomp's generated
native routines alongside its 65816 interpreter safety net. Your own clean ROM
is required. The executable does not depend on Mesen; it is a development reference.

## Current progress - October 17, 2026

This is a playable development port with **375 AOT variants, 123 declared LLE
variants and 258 analysis roots**. All 369 prior native variants remain. The
interpreter safety net stays enabled; counts are not completion percentages.

Audited Items/Magic/Equipment closing now takes **32 fields** in room 01 and
outdoor room 02, **33 in Atland**, down from 52/53. The 20-field improvement
retains every original brightness write, upload and queue-full wait. Openings
remain 40/38/39 fields. Unknown contexts retain the original behavior.

Five original boss child states now execute natively through the original
dispatcher. An explicitly synthetic actor-state fixture exercises all 99 source
instruction keys and removes 514 interpreted executions, 482 excluding returns.
A separate original Samson defeat fixture also executes a guarded four-operation
native cleanup after its real wait; the following audio call remains interpreted.
These bounded tests do not establish natural campaign progression or full native
death/boss orchestration.

Batch 17 passes 22 fresh matched comparisons covering 71,121 ordered frames
and 66 complete state/clock checkpoints. The exact tested executable is selected
and passes an isolated cold menu replay. Settings and saves stay local.

Room 07 now widens its independently scrolling background using the original
quarter-speed scroll and guarded static 32×64 BG2 map. Central and world-edge
samples resolve to original background/OAM colors or required black. Elf Field
Start/fade and the existing menu layouts retain their earlier guarded support.
Broader scripted scenes, transitions and unsupported resources still fall back.

Widescreen supports 16:9, 16:10, 21:9, 32:9 and window-driven aspects in audited
areas, with original pixels and guarded world/actor composition. Supported town,
interior, outdoor and cave menus keep proven scenery behind them. Unsupported
resources retain reference output. Smooth/bilinear scaling was removed; optional
Enhanced original pixels remains available. The independent live custom renderer
retains its stable Atland scope; the reference PPU still runs. This is original
artwork, not a full retexture or a D3D12 backend. No locked 60 FPS claim is made.

Existing assisted encounter checks reach 73 of 75 room records, defeat seven
main bosses with original rewards/flags, exercise all six Thimale guardians,
and cover six endgame phases. Those completed explorations are reused for
focused regression checks rather than repeated as a natural playthrough.
The owner and friends will perform the uninterrupted start-to-finish playtest.
Records 2A/3F, broader native death/boss orchestration, remaining widescreen gaps,
optional soundtrack mappings and hardware/controller qualification remain open.

Read [batch 17](NATIVE-BATCH17-2026-10-17.md),
[current status](docs/DEVELOPMENT-STATUS.md), [menu details](docs/FAST-MENUS.md),
[widescreen scope](docs/WIDESCREEN.md), [custom renderer](docs/CUSTOM-RENDERER.md),
[performance evidence](docs/PERFORMANCE.md), [save states](docs/SAVE-STATES.md)
and the [todo list](TODO.md). Prior campaign exploration and the canonical
coverage corpus are historical evidence; this batch does not relabel them.

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
settings, all ten QoL toggles and experimental widescreen aspects. Apply/Revert
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
