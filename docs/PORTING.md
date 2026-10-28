# Linux and macOS

The interpreter is part of the port, not a Windows dependency. Complete native
coverage is not required for Linux or macOS. SDL3, OpenGL, the generated game
code and the interpreter are compiled for the target machine. Unsupported guest
paths retain their existing guards/fallbacks.

## Current qualification

The stable Windows0.2.5 game remains unchanged. Experimental Linux x64, Intel
Mac and Apple Silicon Mac packages are available in the
[platform preview release](https://github.com/rages4calm/LagoonSNESRecomp/releases/tag/v0.2.5-ports-preview.1).

Hosted Ubuntu24.04 and macOS15 checks passed: native generated-game build/link,
real OpenGL3.3+ context/draw/readback, POSIX profile locking/achievement worker
commit/reload, native settings/input/reward transactions, headless UI pages,
dependency auditing and a real window from the actual self-contained packaged
Companion. Target gameplay and real audio/controllers have not received a full
playthrough here. The successful Windows campaign is not a Linux/macOS campaign.

Extract the entire platform tar.gz, open Start Lagoon.command (or execute
./lagoon.Companion), select/Apply your clean USA ROM, then Game setup/Run setup.
Python3.11+ is required for first-run setup; the .NET runtime is bundled and no
compiler is required for these prebuilt downloads. Config, saves, map and
achievements stay in local/. Keep the extracted folder together.

Linux binaries are built on Ubuntu24.04 and need a compatible desktop runtime
and OpenGL3.3+. Mac builds were checked on macOS15; they are not notarized.
Use the OS's normal per-app approval if first launch is blocked. These are
folder packages, not a notarized drag-to-Applications app bundle.

`Linux and macOS portability checks` is a manual GitHub Actions workflow for
Ubuntu24.04 x64, macOS15 Apple Silicon and macOS15 Intel. It builds the actual
authored game/renderer/interpreter host as a ROM-free setup host. That executable
cannot boot Lagoon and is never published as a playable release. Only its
compile-only metadata is uploaded. No ROM, generated game sources, saves or
private validation inputs are uploaded by this public workflow. A separately
owner-authorized private build repository may consume frozen generated C to
compile playable platform binaries. The ROM and owner saves never leave the PC;
generated C is excluded from public source and all release archives. The older release workflow remains a
legacy setup-pack path; it is not the playable Companion package pipeline.

## Build the playable game on its target OS

Install Python3.11+, CMake3.20+, Ninja and a C/C++ compiler (Clang recommended).
macOS needs the Xcode command-line tools. Linux needs OpenGL development headers
and the X11/Wayland/audio headers listed in `.github/workflows/ports-check.yml`.
The portable build helper enables `LAGOON_PORTABLE_STATIC_SDL3`, compiling the
pinned SDL3 release statically even when a system SDL is installed. This avoids
silently depending on a developer's Homebrew or distribution SDL library. A
developer can explicitly disable this CMake option to use an installed SDL3;
then check its runtime shared-library requirements before packaging. The helper
explicitly selects `SNESRECOMP_FRAME_IMPL=HLE` and
`SNESRECOMP_EXECUTION_DEFAULT=off`, matching the shipped Windows configuration.
Here HLE means frame-composition staging; it does not disable the CPU interpreter
or imply complete native gameplay coverage. Compiling that presentation path on
a target OS is distinct from passing target gameplay/visual qualification.

The Linux launcher omits exactly the optional .NET
`libcoreclrtraceptprovider.so` LTTng diagnostics provider. CoreCLR loads it with
a nonfatal `dlopen`; ordinary application operation does not require that
provider or a legacy LTTng ABI. All selected native runtime libraries still pass
the strict dependency audit. This omission does not disable ordinary game audio,
graphics, achievements or managed application logic. See the upstream
[CoreCLR tracepoint loader](https://github.com/dotnet/runtime/blob/v8.0.21/src/coreclr/pal/src/misc/tracepointprovider.cpp).

Clone with submodules. Use the committed framework revision and cumulative patch;
do not reset or update a dirty existing SDK. From the source checkout:

```sh
python3 -P scripts/build_port.py --rom '/absolute/path/Lagoon (USA).sfc'
```

Generation reads/verifies your own clean USA ROM locally. It uses the committed
campaign analysis roots and title patch policy. The generated program stays in
ignored `src/gen`; never commit it. The build uses ignored `build-port/`, keeping
the existing `build/` untouched. An existing locally generated program can be
reused with `--reuse-generated`. Pass extra CMake options with, for example,
`--cmake-arg=-DCMAKE_OSX_ARCHITECTURES=arm64`. To opt into a system SDL3 instead,
pass `--cmake-arg=-DLAGOON_PORTABLE_STATIC_SDL3=OFF` and its `SDL3_DIR`.

For a ROM-free compile check only:

```sh
python3 -P scripts/build_port.py --setup-host
```

This uses a separate empty generation directory. CMake emits `lagoon-build.json`
with `playable:false` for a setup host. The portable packager refuses it. A real
generated build records its generated-manifest and framework-patch SHA256s.
Those identify the compiled inputs; they do not prove gameplay correctness.

Build/publish the portable Avalonia Companion described in `launcher/README.md`
for the same runtime identifier (`linux-x64`, `osx-arm64` or `osx-x64`). Use a
self-contained directory publish; the package must contain `libcoreclr.so` or
`libcoreclr.dylib`. The Windows Forms `net8.0-windows` launcher is not a Unix executable.

## Stage a prebuilt package

After native OS tests pass, stage explicit inputs into a new directory:

```sh
python3 -P scripts/package_portable.py \
  --platform linux-x64 --stage dist/lagoon-linux-x64 \
  --game build-port/LagoonSNESRecomp \
  --build-metadata build-port/lagoon-build.json \
  --generated-manifest src/gen/program_manifest.json \
  --launcher /path/to/fresh/self-contained/publish \
  --companion-root /path/to/CompanionLauncher \
  --sdl-license build-port/_deps/sdl3-src/LICENSE.txt \
  --zlib-license /path/to/zlib/license-notice.txt
```

Use `macos-arm64`/`macos-x64` on those targets. ELF/Mach-O type and CPU architecture
are checked for both game and launcher. The packager refuses a renamed Windows
binary, a setup host or stale generated-manifest/patch metadata. Generated C and
the ROM are not copied. Catalog, shader, achievement/cameo and dependency notices
are explicit reviewed inputs; saves, local configuration and debug output are
excluded. Optional `--soundtrack-root` accepts only the supplied complete40-track
album with matching import hashes; otherwise the package retains SNES music.
SDL3/zlib license paths must name the exact build dependencies' notices. Exact
self-contained .NET runtime notices come from the target NuGet runtime packs;
use `--nuget-packages` when that cache is in a custom location. Package creation
fails when those notices are missing rather than silently borrowing Windows ones.

`portable-package.json` is the payload hash contract. The setup script checks it
and the ROM before initializing `local/`, preserves existing settings/saves and
reads the ROM in place. Companion runs from `local/`, with `--config config.ini`
and `--save-root saves`; short relative save paths avoid the framework's bounded
save-root buffer. Runtime assets remain under `game/assets`, and the mod catalog
is initialized under `local/mods`. A user selects their own ROM in Companion.

Prefer a `tar.gz` produced on the target OS so executable permission bits survive.
Before release, inspect `ldd game/LagoonSNESRecomp` on Linux or `otool -L` on macOS,
and the launcher dependencies. Do not assume a development-machine Homebrew/SDL
library exists on a player's system. Native package creation currently does not
vendor arbitrary shared libraries or sign/notarize a macOS app. Do not tell users
to disable Gatekeeper globally. Signing/notarization and clean-machine install
tests are separate release steps.

## Required target checks

Use a fresh writable package folder with your own ROM. Check first-run setup,
launch, intro-to-Atland, movement, sword, a room transition, map/settings open and
close, save/load, controller input, SNES and optional X68000 audio, then quitting
and reopening without lost progress. A repeat campaign is unnecessary unless a
new defect warrants it. Keep all personal saves separate from fixtures and
record tested OS/architecture/configuration. Intel Mac results do not certify
Apple Silicon, and a setup-host compile does not certify either gameplay path.

Runner references: [GitHub hosted runners](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
