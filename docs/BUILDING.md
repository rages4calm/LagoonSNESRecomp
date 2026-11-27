# Building Lagoon Recomp

## Prebuilt releases

Windows **0.6.0** provides one Companion for **SNES Enhanced** and **X68000
Enhanced**. Its SNES engine is prebuilt; its X68000 engine is generated locally
from the player's disks, with compatible firmware obtained by setup. Linux x64, Intel Mac and Apple Silicon
downloads remain **SNES-only 0.5.3**. [Downloads](../README.md).

SNES setup needs your clean original USA ROM and no game compilation. Windows
bundles .NET and needs no Python installation for SNES. Linux/macOS first-run
SNES setup uses Python 3.11+, with .NET bundled. X68000 setup automatically
obtains its portable Python/compiler tools; no manual Python or Visual Studio
installation is required.

Extract the whole package into a writable folder and choose your ROM in
Companion's **SNES Enhanced** picker. Verification/setup run automatically. SNES runtime files stay under `game/`; settings,
mod preferences and saves use `local/`. Setup verifies the payload and ROM,
reads the ROM in place and preserves existing local progress. Keep the folder
together. [Playing](PLAYING.md) · [Platform details](PORTING.md).

SNES release execution is strict native for the main 65816 CPU. The interpreter
is an explicit developer reference mode. Graphics, DMA, timers and SPC700/DSP
audio still use the hardware runtime.

Windows uses the title-owned D3D11 flip presenter by default, with GPU EPX or MMPX.
It links the Windows `d3d11`, `dxgi` and `d3dcompiler` libraries. OpenGL remains
available; Linux/macOS use it. Detached world-view composition reads immutable
qualified scene packets on the CPU; display scaling and Enhanced pixels run on
the GPU. Expanding the presentation does not enlarge the guest PPU raster.

## X68000 local build and setup

The Windows package includes the X68000 enhanced runtime source and setup
recipe. It cannot include a prebuilt X68000 game: RetroRecomp's exported
executable embeds the game disks and firmware. Those inputs and the resulting
export remain on your PC.

1. Extract the full Windows release into a writable folder and open Companion.
2. Choose **X68000 Enhanced**, then select your supported four-disk **ZIP or folder**.
3. Click **Set up game**. Setup downloads verified portable tools, compatible
   firmware and the English patch when needed, then builds
   the English enhanced game. Keep the setup/log window open until it completes.
4. Press **Play**. Future launches use the built game; changing edition does
   not rebuild it.

The required game input is four **DIM disk images**. Setup checks content,
rather than trusting names alone. Normal setup obtains compatible IPL firmware
and a public-domain Shinonome font from the pinned WebX68k source, with notices.
There is no firmware-file hunt. **Use custom firmware (advanced)** optionally
accepts **iplrom.dat (128 KiB)** and **cgrom.dat (768 KiB)** instead.
It accepts the pinned English 0.9b System disk or applies nleseul's patch to the
supported Japanese System disk. Lagoon game disks are never downloaded.
The translation and official **RetroRecomp 1.8.0** converter are downloaded
when needed and verified before use. The portable LLVM-MinGW/CMake/Ninja tools
require internet and local disk space on first setup, without administrator
installation. Tools are cached for later use.

An alternative is to place the ZIP or extracted disks under
**Inputs/X68000/**, then run **Setup X68000.cmd**. From an extracted package,
the noninteractive launcher entry is:

```powershell
.\lagoon.Companion.exe --x68000-setup --inputs "C:\My Games\Lagoon disks.zip"
```

The enhanced export and receipts go under
`local-x68000/retrorecomp-expedition/`. Keep that directory together. Saves and
preferences live in its `datas/` tree. The public recipe uses
`x68000/tools/build_retrorecomp.py` and `build_atlas_player.py`, with title-owned
rendering under `x68000/runtime/` and guarded game changes in `lagoon_qol.py`.

Optional CLI arguments `--ipl FILE --cgrom FILE` select custom firmware. The
normal ZIP-only command obtains the compatible pair automatically.

If setup stops, read the first error in its log. Unsupported/missing disk
images need the supported four-image set. A custom-firmware size error needs
the actual 128 KiB IPL and 768 KiB CGROM files, not renamed ZIPs; otherwise leave
the advanced option off and use automatic firmware. Connection
failures can be retried through **X68000 setup**. Close a running X68000 game
before rebuilding it or copying its saves. Keep any existing User Disk and
use Companion's ordinary-save import; an old bootable disk also contains old
game files and must not replace the new patched template.

No source-identity, checksum, mode or snapshot guard should be disabled to
force a build or old state to load. X68000 retains the runtime/interpreter
compatibility support; the SNES native-CPU claim does not describe this edition.

## SNES source checkout

Install Python 3.11+, Rust, CMake 3.20+, Ninja, a C/C++ compiler, SDL3 and zlib as
required by the pinned [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp)
source. macOS needs the Xcode command-line tools. Commands assume the repository
root; Windows regeneration uses Git Bash.

For a fresh checkout, initialize submodules, then select the pinned SDK in a
separate worktree before applying the cumulative patch.
Preserve a dirty existing SDK: never reset or bootstrap it to resolve a mismatch.
Select a separate reviewed checkout through `framework.local`,
`LAGOON_FRAMEWORK_ROOT` or CMake's `SNESRECOMP_ROOT` when necessary.

```sh
git submodule update --init --recursive
git -C snesrecomp fetch origin 22c01ef12cc94767451d0b218e4fc341bc19043e
git -C snesrecomp worktree add --detach ../../lagoon-sdk 22c01ef12cc94767451d0b218e4fc341bc19043e
printf '../lagoon-sdk\n' > framework.local
python -P tools/apply_framework_patch.py
bash tools/regen.sh --rom "/absolute/path/Lagoon (USA).sfc"
cmake -S . -B build-native -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLAGOON_GENERATE_NATIVE69_FROM_ROM=ON \
  -DLAGOON_NATIVE69_ROM="/absolute/path/Lagoon (USA).sfc" \
  -DLAGOON_NATIVE_RELEASE_DEFAULT=ON -DLAGOON_REQUIRE_NATIVE_CPU=ON
cmake --build build-native -j
```

The ordinary generated program and both architectural native profiles are
required for a playable release. The profile generator checks the ROM, compiler
pins and camera contract before writing private C under the build directory.
[Native profile inputs](../config/native69/README.md). Release defaults enable
native execution and strict refusal: an uncovered or mismatched instruction
stops before interpretation. Keep source, mapping, mode, stack and generation guards.

`framework_pins.txt` records the baseline; `patches/lagoon-framework.patch` carries
Lagoon's cumulative changes. The selected revision is
`22c01ef12cc94767451d0b218e4fc341bc19043e`. The patch helper refuses other
revisions; `python -P tools/apply_framework_patch.py --print-root` reports its
selection without applying changes. A changed compiler/ABI needs review and
qualification before native metadata pins are updated.

The required ROM is an unheadered 1 MiB USA LoROM, SHA256
`46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb`,
CRC32 `d2554270`. Patched ROMs are refused. ROMs, generated C, saves, extracted
Lagoon assets and private validation remain local. Release executables contain
compiled code; archives do not include generated sources or the ROM.

## Companion and sidecars

Build CompanionLauncher's pinned 0.2.0 local packages, then:

```powershell
powershell -File scripts/build_launcher.ps1 -CompanionRoot C:\path\to\CompanionLauncher
```

Windows uses `launcher/Game.Companion.csproj`; Linux/macOS share the adapter
through `launcher/desktop/Game.Companion.Desktop.csproj`. Publish self-contained
for the target runtime. [Adapter details](../launcher/README.md).

Deploy matching mod manifests, EPX shaders, achievement catalog, credited cameo
and provenance, icons and dependency notices with the executable. An updated
EXE with an old catalog can reject valid QoL selections. Check sidecars with:

```sh
python -P tools/check_runtime_catalog.py --runtime-root build-native
```

For a package, select `path/to/package/game` and add
`--writable-root path/to/package/local`. This checks sidecars without modifying
preferences; it does not qualify gameplay.

## Target builds and packaging

On Linux/macOS, `python3 -P scripts/build_port.py --rom '/absolute/path/Lagoon (USA).sfc'`
generates both profiles and builds strict native execution in `build-port/`.
Private builders can use `--native-generated-dir` with reviewed frozen profiles
and the matching ordinary generated program. The ROM-free `--setup-host` mode
is compile-only and cannot play Lagoon.

Use explicit runtime inputs with `scripts/package_windows_preview.py` or
`scripts/package_portable.py`. Their historical filenames do not define the
release channel. Pass `--require-native` to both packagers and
`--build-metadata path/to/lagoon-build.json` for the exact compiled executable.
Windows native enforcement requires that metadata argument too. They stage
payload hashes, licenses and runtime files, excluding ROMs, generated source,
developer configuration and progress. Never publish a
developer build tree. [Target qualification and package checks](PORTING.md).

## Developer reference execution

Select the interpreter explicitly for a matched comparison:

```sh
LAGOON_NATIVE_BACKEND69=0 SNESRECOMP_NATIVE_ONLY=0 ./build-native/LagoonSNESRecomp --no-launcher
```

Use isolated settings/saves and identical source, initial state and input timing.
A deliberately non-native development build also needs both
`LAGOON_REQUIRE_NATIVE_CPU=OFF` and `LAGOON_NATIVE_RELEASE_DEFAULT=OFF`.
[Development and verification](DEVELOPMENT.md).

## Presentation design

Windows uses a title-owned D3D11 windowed flip presenter; Linux/macOS retain
OpenGL. Enhanced pixels offers EPX and MIT-licensed MMPX reconstruction in
HLSL and GLSL, followed by sharp fractional scaling of the reconstructed 2x
lattice. MMPX follows the credited McGuire/Gagiu rules; its separate C99 oracle is in
`third_party/mmpx/`, with the original port's upper-border clamp corrected. Only
a one-output-pixel edge footprint blends: integer-aligned output
remains exact. The 34-row HUD frame and the title-owned menus bypass edge
reconstruction. D3D11 receives the footprint from the actual viewport size,
avoiding derivatives across the HUD's divergent shader branch.

This follows the useful 2D presentation approach in
[Babel](https://github.com/mrOt4/babel-recomp) and the filtering choices in
[EarthBound Companion](https://github.com/rages4calm/earthbound-companion).
[Star Fox Enhanced](https://github.com/kandowontu2/starfox-enhanced) also uses
higher-resolution 3D geometry, lighting and shadows; those do not add detail to
Lagoon's original 2D tiles. No code from those projects was copied. The graphics
API itself does not replace artwork or increase gameplay accuracy.

`lagoon_d3d11_test` reads back actual GPU pixels and checks integer EPX, sharp
fractional output, HUD/menu bypass, zero-alpha guest input and resize/letterbox.
`tests/lagoon_world_view74_test.c` checks detached-world composition without ROM
inputs; focused gameplay and OBS focus-switch recordings are separate checks.
