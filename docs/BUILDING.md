# Building Lagoon Recomp

## Prebuilt releases

The v0.3.0 release line targets Windows x64, Linux x64, Intel Mac and Apple
Silicon Mac with the actual prebuilt game, self-contained Companion and converted
X68000 album. Check the [main page](../README.md) for publication status and downloads.
Supply your own clean USA ROM. No game compilation or source generation is needed
to play these packages. Windows setup needs no Python or .NET installation;
Linux/macOS first-run setup uses Python 3.11+, with .NET bundled.

Extract the whole package into a writable folder, select/Apply the ROM in
Companion, then run Game setup. Runtime files stay under `game/`; settings,
mod preferences and saves use `local/`. Setup verifies the payload and ROM,
reads the ROM in place and preserves existing local progress. Keep the folder
together. [Playing](PLAYING.md) · [Platform details](PORTING.md).

Normal release execution is strict native for the main 65816 CPU. The interpreter
is an explicit developer reference mode. Graphics, DMA, timers and SPC700/DSP
audio still use the hardware runtime.

## Source checkout

Install Python 3.11+, Rust, CMake 3.20+, Ninja, a C/C++ compiler, SDL3 and zlib as
required by the pinned [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp)
source. macOS needs the Xcode command-line tools. Commands assume the repository
root; Windows regeneration uses Git Bash.

For a fresh checkout, initialize submodules before applying the cumulative patch.
Preserve a dirty existing SDK: never reset or bootstrap it to resolve a mismatch.
Select a separate reviewed checkout through `framework.local`,
`LAGOON_FRAMEWORK_ROOT` or CMake's `SNESRECOMP_ROOT` when necessary.

```sh
git submodule update --init --recursive
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
`98f41838443a7cd7f9d56bec41387a6ebfc3bcbc`. The patch helper refuses other
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
release channel. For v0.3.0, pass `--require-native` to both packagers and
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
