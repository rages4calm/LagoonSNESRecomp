# Lagoon Recomp

A work-in-progress Windows port of Lagoon (USA), using SNESRecomp's generated
native routines alongside its 65816 interpreter safety net. Your own clean ROM
is required. The executable does not depend on Mesen; it is a development reference.

## Current progress — October 15, 2026

Four additional gameplay routines now run as generated native code:

- Entity movement integration (`01:A4EB`).
- Room-boundary clamping (`01:A56A`).
- World-to-screen projection and visibility (`01:A5AA`).
- Direction/speed-tier velocity selection (`02:80EA`).

They use opt-in instruction-boundary timing and real bus-region costs. The
7,219-frame movement/menu route matches the prior native control at all 18 dump
tags, including framebuffer, WRAM, registers, VRAM, OAM, CGRAM and guest clock.
Interpreted work falls from 72,427,712 to 70,655,953 instructions on that route.
That is 2.45% less interpreted work, not a native-completion percentage.

The build generates 147 AOT variants and 18 declared LLE variants; additional
runtime fallback remains available. The original opening-loop fix stays in place.
The tile-row staging routine remains interpreted after a failed timing retry.
Existing native-versus-pure-interpreter clock/state differences remain unresolved.
Combat, progression, save/load, the full campaign and widescreen are not qualified.
See [the evidence receipt](NATIVE-PROGRESS-2026-10-15.md) and [todo list](TODO.md).

## Launcher

Use `Start Lagoon.cmd` after building. It opens the shared
[Companion Launcher](https://github.com/rages4calm/companion-launcher), using pinned
0.2.0 packages. The Lagoon adapter verifies your ROM and starts the native game
with its existing config and save locations. The old pre-boot UI is excluded from
the default build; it can be enabled explicitly for development.

The initial Companion adapter exposes executable, ROM and native config paths.
Display/audio/input controls and the QoL mod page still need full adapter integration.
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

Existing optional walk-speed, camera and starting-stat plugins remain default-off.
The supplied QoL BPS now decodes correctly and provides addresses for further
work. It also contains FastROM remapping and instruction changes, so loading the
whole patch into the interpreter alone would conflict with generated native code.
Its data informs the implementation; it is not bundled or automatically applied.

All requested changes, including sword reach, followers, recovery, starting stats,
EXP, jump airtime, magic and modern widescreen aspects, are tracked in `TODO.md`.
They are not all implemented. See `QOL-ADDRESSES.md` for the current reference.

## Source and publishing

This repository contains source, build configuration and development tools. It
excludes ROMs, generated ROM-derived C, private decompilation exports, extracted
assets, saves, states, screenshots and test captures. No playable release is being
published with this source snapshot. GitHub updates are deliberate batch snapshots,
not an automatic per-feature sync.

Retain the included licenses: the SNESRecomp foundation and its derivatives use
PolyForm Noncommercial; Companion Launcher and the Lagoon launcher adapter use MIT.
Public source availability does not remove the framework's noncommercial terms.
