# Lagoon Companion adapter

Uses the shared Companion Launcher 0.2.0 packages; no shared UI source is copied.
Build with `powershell -File scripts/build_launcher.ps1 -CompanionRoot <checkout>`
from the game repository. The build script finds the local package feed and
restores the pinned lock file. Developer builds need .NET 8 Windows Desktop
Runtime. The public Windows release is self-contained and needs no .NET install.

## Windows edition selection

Windows 0.6.1 uses this same Companion executable for **SNES Enhanced** and
**X68000 Enhanced**. The first screen selects the edition; the sidebar's
**Choose edition** returns to it. **Remember my choice** is optional. The
explicit entries are `--game snes`, `--game x68000` and `--game ask`.
The chooser does not redirect into a separate LagoonExtra checkout.

Selecting an unconfigured SNES edition opens the existing ROM picker.
Selecting an unbuilt X68000 edition opens its disk/firmware setup form. The
X68000 **Set up game** action streams setup output and returns to its Companion
page after success. **X68000 setup** retries it; **Copy X68000 saves** opens
the ordinary-slot import tool. Editions retain separate settings and progress.

## SNES adapter

For a release package, open `Setup Lagoon.cmd`. The first-run ROM picker verifies
your original USA .sfc/.smc file and runs setup automatically. Use `Choose ROM.cmd`
or `Choose ROM.command` to reopen it later. The game EXE is bundled
under `game/`; setup needs no compiler, conversion or download. Config, shaders,
mod preferences and saves stay under `local/`. Companion passes that config and
an explicit local save root. Packaged files remain unchanged.

The Game page selects the native executable, original USA ROM and existing native
config.ini. Play verifies ROM SHA256, supplies structured `--no-launcher`, `--rom`
and `--config` arguments, and retains the executable directory as the working
directory for development builds. Native saves remain in the existing game save directory. Launcher
path preferences use Companion's own storage; no save or engine setting migration
is performed.

Display, Audio and Input read the selected native INI directly. Supported controls
are window/fullscreen mode, window scale, enhanced pixels, vertical sync, audio enable,
volume, player-one gamepad enable and analog deadzone. Existing keyboard bindings,
button mappings, controller GUIDs and unknown fields are retained. Apply writes only
changed native controls; restart the game to use them. Selecting another executable
or INI and applying loads that source's settings without writing the previous
source's values into it. Make further setting edits after that refresh.

**Enhanced original pixels** offers EPX and MMPX with sharp fractional scaling.
Windows uses D3D11 by default, with OpenGL available; Linux/macOS use OpenGL.
The selected shader assets must accompany the executable. New release display
defaults come from the packaged INI; existing native preferences are preserved.
Turning the enhancement off restores the previous shader, renderer and output
method stored in native INI metadata. Bilinear Smooth scaling has been retired; Apply keeps
`LinearFiltering = 0`, including when old EPX metadata previously saved smoothing.
Other
settings, bindings, mod selections and saves remain in place. Restart after Apply. This is a
pixel treatment, not replacement artwork or smoother gameplay motion. See
[display settings](../docs/PLAYING.md#snes-display).

Widescreen offers 16:9, 16:10, 21:9, 32:9 and follow
window choices through `[Graphics] Widescreen` and `[Lagoon] WideAspect`. It expands
qualified scenery and actors. The release INI enables it; existing settings are
preserved. Unusual effects/layouts retain separate coverage
requirements; unsupported visual content keeps its original view.

Audio has one **Soundtrack** dropdown: **Original SNES** (default) or **X68000**.
Choose one, Apply and restart. The release's complete X68000 pack is selected
automatically from `soundtrack/x68000`; there is no folder field or conversion.
Developer builds also find the existing workspace's `Soundtracks/X68000` import.
Missing/unmapped cues retain SNES audio and sound effects keep the original
driver. Master and X68000 music-only volume are separate controls; R2 or Ctrl+comma
opens the frozen in-game volume panel without returning to the launcher.
The selection persists in the matching `lagoon.soundtrack` native mod
feature/resource. See [soundtrack details](../docs/SOUNDTRACK.md).

The Quality of life page changes the matching native package's feature selections
in `local/mods/preloaded/state.toml` for release packages, or beside the executable for
development builds. Gameplay toggles default off: walking speed, opening balance,
camera centering, sword reach with modest lateral cleave, follower speed, recovery
time, jump peak hold and magic strength. Separate EXP and gold dropdowns offer
original rewards or 2x through 8x. EXP also keeps the previous 1.5x setting, read
from existing `exp_boost` preferences. The dropdown clears the preceding reward
selection; multipliers never stack. These change enemy/boss reward data only,
leaving shop prices, quest rewards and existing saved totals untouched. Menu responsiveness
is built into the port. Giles first aid is also built in: accepting his rescue request
does not require or consume a Healing Pot. The old fast-menu flag remains a harmless compatibility
alias for existing catalogs. Gameplay features require the
matching `lagoon.quality/1.0.0/manifest.toml` package. These are native feature
switches; the original ROM remains clean. Opening balance affects new games.

Revert reloads authoritative settings. Revisions cover launcher preferences, INI,
mod state, package manifests and executable-relative `keybinds.ini`; external edits
require Revert before Apply. Keyboard and Hotkeys pages edit the native key names
and system shortcuts. Controller has one button selector per SNES input; L2 is
reserved for the map and R2 for quick in-game settings. Existing GUIDs, player-two
bindings and unknown fields remain intact. Keyboard/config/mod edits share the
same atomic-write rollback transaction. Changed
files use atomic replacement, with rollback if a later commit fails. This does not
provide filesystem-wide atomicity during power loss. Unreadable settings and invalid
known values are rejected rather than replaced with defaults. Unknown launcher JSON,
INI and mod-state fields survive. The SNES executable defaults to strict native
main-CPU execution, stopping before an uncovered instruction instead of using
an automatic interpreter fallback. Explicit developer overrides can select
the reference interpreter. Graphics/audio hardware still use the runtime.

`--smoke-test <fresh-directory>` renders all declared pages and exercises draft/Revert
behavior without launching the game. `tests/Lagoon.Launcher.Tests.csproj` checks
clean/wrong ROM readiness, structured arguments and an isolated 120-frame native
startup. It also exercises real synthetic Apply/Revert, source switching, stale INI
and mod-state writes, unknown-field/BOM/newline preservation, malformed state, absent
native packages, and default-off QoL. Its arguments are
`native-exe clean-rom fresh-output-directory`. Runtime startup uses a private working
directory and save root so its explicit config also resolves a private mod catalog.
Use `COMPANION_DATA_ROOT` pointing to the synthetic launcher-data root for UI smoke
tests. Screenshots and synthetic test fixtures stay outside the public repository.

The shared packages and this adapter are MIT licensed; see LICENSE.

## X68000 adapter and local setup

X68000 is currently Windows-only. It has one fixed **X68000 Enhanced** gameplay
edition, rather than selectable QoL profiles. Companion exposes display
preferences; the runtime supplies live audio/zoom settings and remappable
controls. The default new-install view is 1.25x, square pixels, with audio at
80%; subsequent setup preserves an existing preferences file.

The setup form requires only a supported four-disk ZIP/folder. Compatible
firmware is obtained automatically; custom IPL/CGROM files are optional under
**Use custom firmware (advanced)**.
Portable Python and the verified LLVM-MinGW/CMake/Ninja toolchain are obtained
automatically; players do not need to install Python or Visual Studio. Setup
downloads the pinned official converter, compatible IPL/font firmware and
translation patch when needed, never the Lagoon game. The CLI equivalent is:

```powershell
.\lagoon.Companion.exe --x68000-setup --inputs "C:\Games\Lagoon disks.zip"
```

The local recipe creates `local-x68000/retrorecomp-expedition/`. Play requires
the matching verified base/Atlas receipts, executable hashes, source identity
and 49-area catalog. The Atlas player is compared with the verified base's
opening before installation. A missing or stale export leaves Play unavailable
with setup guidance. Its executable embeds the supplied disks/firmware and
must remain private; public packages contain source/setup, not that payload.

User Disk data lives below the edition's `datas/games/x68-<identity>/` folder.
The copy tool validates the source/destination format, requires an empty target
slot and holds the runtime's exclusive session lock. It copies a 1 KiB save
sector, not the whole bootable User Disk. That preserves updated game scripts.
Source identity and quick-state checksum/layout checks remain in place;
ordinary save imports do not bypass snapshot compatibility. Progress and
achievements never cross into the SNES edition.

X68000 uses its compatibility/runtime support and is not described by the
SNES 65816 native-CPU claim. See [player controls and saves](../docs/PLAYING.md#x68000-enhanced)
and [local setup](../docs/BUILDING.md#x68000-local-build-and-setup).

### Windows 0.6.1 updater

**Check for updates** checks this repository's official stable Windows release.
The dialog offers **Download and update**, **Resume update** for an interrupted
apply, and **Official releases**. A failed online check leaves installed Play
available. Version 0.6.0 users first extract the complete 0.6.1 ZIP over their
release folder with both games/Companion closed, retaining `local/` and
`local-x68000/`, then reopen Companion.
Automatic application is limited to extracted release folders; source checkouts
can check/open the release without replacing the Git working tree. The package
update preserves both `local/` and `local-x68000/`, including the private compiled
X68000 game. Games must be closed before application; the helper waits for
Companion to exit and does not force-kill running programs.

**Update X68000 game** is separate from package application. Runtime source
changes need a local rebuild using cached game inputs and tools when present;
missing inputs require selecting the game ZIP again. Launcher/docs-only changes
do not require rebuilding the private game. Existing supported installations
remain playable until the player chooses to update that runtime.
The dialog's **Update game** button starts the cached rebuild, opens normal
ZIP setup if reusable inputs are absent, or reports **Already up to date**
without rebuilding when the installed runtime matches.

The corresponding presentation change keeps the title/prologue at normal speed
unless explicitly skipped. Windowed and Borderless fit use proportional fit;
Borderless pixel perfect uses integer scaling. Stretch does not distort the
opening. Existing size preferences are preserved; new windows use up to
1280×720 within the available desktop area. Gameplay canvas/zoom remains separate.

## Linux and macOS Companion

These packages remain SNES-only version 0.5.3; their existing portable UI is
unchanged by the Windows X68000 chooser.

The existing Windows Forms launcher stays in Game.Companion.csproj. The portable
Avalonia shell is desktop/Game.Companion.Desktop.csproj, sharing GameAdapter,
NativeSettings, input/hotkey definitions and achievement browsing. It uses the
pinned Companion.Launcher.Avalonia0.2.0 package and Avalonia12.1.4. Build the local
Companion Core/Avalonia packages first and set COMPANION_LAUNCHER_FEED to their
packages directory (see CompanionLauncher README).

```sh
dotnet publish launcher/desktop/Game.Companion.Desktop.csproj -c Release \
  -r linux-x64 --self-contained true -o build-port/launcher
```

Use osx-x64 or osx-arm64 on the matching Mac. A source checkout expects its native
executable at build/LagoonSNESRecomp; a portable package detects
portable-package.json and uses game/LagoonSNESRecomp plus writable local/. Setup
invokes python3 scripts/setup_portable.py --root PACKAGE --rom YOUR_ROM.
Python3.11+ is needed for first setup; .NET is self-contained in release packages.
Play preserves the short relative save-root and shared native INI/mod contracts.
Wrong-OS/architecture packages, changed executables and incomplete setup are
refused. Target-OS builds and hardware gameplay qualification are separate checks.
