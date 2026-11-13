# Windows, Linux and macOS

## Release scope

The v0.5.0 release provides the prebuilt game and Companion for:

| Target | Game architecture | Launcher |
| --- | --- | --- |
| Windows | x64 | Self-contained Windows Companion |
| Linux | x64 | Self-contained portable Companion |
| macOS | Intel x64 | Self-contained portable Companion |
| macOS | Apple Silicon arm64 | Self-contained portable Companion |

Publication status and downloads are on the [main page](../README.md). All four
builds execute main 65816 CPU code natively by default, with strict refusal
instead of automatic interpreter fallback. Graphics, DMA, timers and SPC700/DSP
audio remain hardware-runtime responsibilities. Native CPU execution does not
imply a matching handwritten C decompilation.

Each package includes the actual game, Companion, runtime assets, licenses and
optional owner-approved X68000 album. Supply a clean USA ROM. No ROM, generated
C, personal saves or private fixtures are published in source or release archives.

Extract the entire archive into a writable folder. Windows starts through
`Setup Lagoon.cmd`; Linux/macOS use the packaged Companion/start command.
Select/Apply the ROM, run Game setup, then Play. Linux/macOS first setup needs
Python 3.11+. .NET is self-contained; no compiler or recompilation is required.
Settings, saves, map and achievements stay in `local/`; runtime files stay in `game/`.

## Verification boundaries

The owner completed the Windows campaign and explored every map before the
strict-native migration. Later focused native/reference checks cover the CPU
backend and reported rendering faults; they are not a repeated full campaign.
CPU qualification includes exact state, clock, ordered-frame and offline-DSP
comparisons rather than only successful compilation.

Hosted Linux/macOS checks distinguish native compilation/linking, OpenGL context
and readback, POSIX persistence/locking, settings transactions, UI and packaged
launch from physical hardware gameplay. These checks do not constitute a full
Linux/Mac campaign, real-device audio listening or controller playtest. Intel
Mac results do not certify Apple Silicon, and Windows results do not certify
another OS. Release notes record each artifact's actual checks.

Linux packages target an Ubuntu 24.04-compatible desktop runtime and OpenGL 3.3+.
Mac builds use a macOS 11 deployment target; hosted checks use newer runners.
They are folder packages, not notarized app bundles. Use normal per-app OS
approval if needed; do not disable Gatekeeper globally. Signing, notarization
and clean-machine hardware tests are separate qualifications.

## Build on the target OS

Install Python 3.11+, CMake 3.20+, Ninja and a C/C++ compiler. macOS needs Xcode
command-line tools. Linux needs the OpenGL, X11/Wayland and audio development
headers in `.github/workflows/ports-check.yml`. Use the committed framework
revision and cumulative patch; never reset a dirty existing SDK.

```sh
python3 -P scripts/build_port.py --rom '/absolute/path/Lagoon (USA).sfc'
```

The helper generates locally, requires both native profiles and selects strict
native defaults. It builds in `build-port/`, preserving the working `build/`.
`--reuse-generated` reuses the ordinary program; `--native-generated-dir
/private/native69` selects reviewed frozen native profile C. Both source sets
must match their declared compiler/ABI and manifest. Generated sources stay private.

The helper uses `LAGOON_PORTABLE_STATIC_SDL3` so packages do not silently depend
on a developer's Homebrew or system SDL. To deliberately use installed SDL3,
pass `--cmake-arg=-DLAGOON_PORTABLE_STATIC_SDL3=OFF` and `SDL3_DIR`, then audit its
runtime dependencies. Additional target options can use
`--cmake-arg=-DCMAKE_OSX_ARCHITECTURES=arm64`.

`SNESRECOMP_FRAME_IMPL=HLE` controls frame-composition staging;
`SNESRECOMP_EXECUTION_DEFAULT=off` is a separate framework option. Neither is
Lagoon's CPU policy. `LAGOON_NATIVE_RELEASE_DEFAULT=ON` and
`LAGOON_REQUIRE_NATIVE_CPU=ON` select and require strict native CPU execution.

For a ROM-free compile check:

```sh
python3 -P scripts/build_port.py --setup-host
```

This writes `playable:false` metadata, cannot boot Lagoon and is refused by the
portable packager. Public compile-only workflows never upload generated game
source. An owner-authorized private build repository can compile frozen C;
ROMs and owner saves never enter it, and only audited runtime payloads leave it.

## Packaging

Publish the portable Companion self-contained for `linux-x64`, `osx-x64` or
`osx-arm64`; Windows uses its Windows-specific host. Include the chosen CoreCLR
runtime and exact notices. [Adapter instructions](../launcher/README.md).
Build metadata records input identities; it is not gameplay proof by itself.

`scripts/package_portable.py` checks ELF/Mach-O architecture, rejects setup hosts
and stale generated/patch metadata, and stages explicit runtime files. The
Windows equivalent is `scripts/package_windows_preview.py`; its legacy filename
does not make the v0.3.0 release a preview. Use `--require-native` for release
packaging, with `--build-metadata` naming the exact build's `lagoon-build.json`.
The Windows packager also requires that metadata when native enforcement is
requested. Generated C, ROMs, local settings and saves are excluded. Soundtrack input must match the supplied album's import hashes.

Payload manifests bind every public file. Setup verifies them and the ROM before
initializing `local/`, preserves progress and reads the ROM in place. Companion
passes the selected config and explicit save root. Short relative save paths
avoid the runtime's bounded save-root buffer.

Prefer target-created tar.gz files on Unix to preserve executable permissions.
Inspect `ldd` on Linux or `otool -L` on macOS for both game and launcher. The Linux
launcher omits only the optional CoreCLR LTTng tracing provider; required runtime
libraries still undergo an audit. Do not assume development-machine libraries
exist on the player's system.

## Focused hardware checks

Use a separate writable test profile to check setup, intro, movement and sword,
a room transition, map/settings open and close, save/load, controller input,
both soundtracks, quitting and reopening without lost progress. Repeat a campaign
only when a new defect warrants it. Report OS, architecture, version/hash,
settings and precise steps. [Bug reports](PLAYING.md#reporting-a-bug).

Release0.5.0 adds D3D11 on Windows, shared EPX/MMPX presentation and detached
world zoom. A bounded cheated door-entry sweep checked expanded rendering in
all74 authored travel areas; the unused no-incoming descriptor3F is separate.
Game Over Continue restored HP and movement. End now includes the original
00:9366 restart jump and9304..932D WRAM-clear instructions in both native
profiles; matched native/reference End replays agree on all recorded frames,
checkpoint components and clocks. These checks do not replace the owner campaign.
