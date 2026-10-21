# Building Lagoon Recomp

The Windows preview release uses Companion's **Game setup** page. Extract the
whole package into a writable folder, open `Setup Lagoon.cmd`, set the absolute
path to your clean USA ROM on **Game**, click **Apply**, then **Run setup**.
The release includes the qualified native game EXE and self-contained Companion
EXE, each with its own icon. No .NET, Git, Rust, Python, compiler installation,
generation or download is needed. You supply the clean ROM, which is verified
and read in place. No ROM, extracted graphics, generated C source or saves
are distributed. The optional X68000 music is bundled; choose **Original SNES**
or **X68000** under Audio and Apply. No folder selection is required. First
setup checks every package file and prepares `local/` for
settings, mod preferences and saves; the packaged game stays under `game/`.
Failure cannot publish readiness. Existing local settings and saves are never
overwritten. Extract upgrades into a fresh folder and preserve your old saves.

Maintainers publish Companion with `dotnet publish launcher/Game.Companion.csproj
-c Release -r win-x64 --self-contained true` (set the Companion local package
feed first), then stage the reviewed source with
`python -P scripts/package_windows_preview.py --stage <fresh-directory>
--launcher <self-contained-publish-directory> --game-exe <qualified-native-EXE>
--companion-root <Companion-source> --toolchain <qualification-toolchain>
--soundtrack-root <complete-immutable-X68000-pack>
--generated-manifest
<qualified-local-program_manifest.json> --package-id <preview-version>`.
Staging copies only explicit runtime inputs, records each file's SHA256, and
includes dependency licenses. It never copies a developer build directory, ROM,
private audio, source generation, settings or saves. The local qualification
manifest is represented only by its SHA256. Audit the complete stage and verify
clean first-run setup, runtime DLL closure and isolated gameplay before creating
or publishing its ZIP. Archive only `windows-preview.json` and its `files` list;
setup's writable `local/` directory must never enter the published archive.

The legacy multi-platform setup-host workflow is not the Windows Companion
preview pipeline and must not be used to publish this preview.

## Developer checkout

Install the Windows toolchain/dependencies documented by
[SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp): Python 3.11+,
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


Build commands assume the repository root. For a clean checkout only, initialise
submodules before applying the patch. In an active development checkout, preserve
local framework modifications and apply only reviewed changes; never reset the
framework to resolve a patch mismatch. See [development](DEVELOPMENT.md).
