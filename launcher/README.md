# Lagoon Companion adapter

Uses the shared Companion Launcher 0.2.0 packages; no shared UI source is copied.
Build with `powershell -File scripts/build_launcher.ps1 -CompanionRoot <checkout>`
from the game repository. The build script finds the local package feed and
restores the pinned lock file. .NET 8 Windows Desktop Runtime is required to run.

The Game page selects the native executable, original USA ROM and existing native
config.ini. Play verifies ROM SHA256, supplies structured `--no-launcher`, `--rom`
and `--config` arguments, and retains the executable directory as the working
directory. Native saves remain in the existing game save directory. Launcher
path preferences use Companion's own storage; no save or engine setting migration
is performed.

Display, Audio and Input read the selected native INI directly. Supported controls
are window/fullscreen mode, window scale, enhanced pixels, vertical sync, audio enable,
volume, player-one gamepad enable and analog deadzone. Existing keyboard bindings,
button mappings, controller GUIDs and unknown fields are retained. Apply writes only
changed native controls; restart the game to use them. Selecting another executable
or INI and applying loads that source's settings without writing the previous
source's values into it. Make further setting edits after that refresh.

**Enhanced original pixels (EPX)** is optional and defaults off. It smooths diagonal
edges using original colors while keeping the HUD sharp. Apply selects the OpenGL
GPU renderer and the matching `assets/shaders/lagoon-epx.glslp` preset. Turning EPX
off restores the previous shader, renderer and output method stored in native
INI metadata. Bilinear Smooth scaling has been retired; Apply keeps
`LinearFiltering = 0`, including when old EPX metadata previously saved smoothing.
Other
settings, bindings, mod selections and saves remain in place. Both EPX shader
files must accompany the selected executable. Restart after Apply. This is a
pixel treatment, not replacement artwork or smoother gameplay motion. See
[enhanced pixel details and bounded checks](../docs/ENHANCED-PIXELS.md).

Experimental widescreen defaults off and offers 16:9, 16:10, 21:9, 32:9 and follow
window choices through `[Graphics] Widescreen` and `[Lagoon] WideAspect`. It expands
backgrounds; off-screen NPC/enemy rendering and some rooms remain unfinished.
Unsupported scenes retain their original view.

Audio also offers **Original SNES** (default) and **X68000 (partial, experimental)**.
Select the private imported music folder, choose X68000, Apply and restart.
The local converted folder is `C:\Users\crono\Desktop\Projects\Lagoon\Soundtracks\X68000`;
VGZ conversion is already complete, so MP3 files are not required. Title, Atland
and opening Gold Cave music are mapped; other/missing cues retain SNES audio and
sound effects stay on the original driver. State loads restart the cue, and
run-ahead/netplay use SNES. The selector and folder persist as the matching
`lagoon.soundtrack` feature/resource in native mod state. Nothing enables it
automatically. See [soundtrack details](../docs/X68000-SOUNDTRACK.md).

The Quality of life page changes the matching native package's feature selections
in `mods/preloaded/state.toml` beside the executable. All nine gameplay options default off:
walking speed, opening balance, camera centering, sword reach with modest lateral cleave, follower speed,
recovery time, EXP rewards, jump peak hold and magic strength. Menu responsiveness
is built into the port. Giles first aid is also built in: accepting his rescue request
does not require or consume a Healing Pot. The old fast-menu flag remains a harmless compatibility
alias for existing catalogs. Gameplay features require the
matching `lagoon.quality/1.0.0/manifest.toml` package. These are native feature
switches; the original ROM remains clean. Opening balance affects new games.

Revert reloads authoritative settings. Revisions cover launcher preferences, INI,
mod state and package manifest; external edits require Revert before Apply. Changed
files use atomic replacement, with rollback if a later commit fails. This does not
provide filesystem-wide atomicity during power loss. Unreadable settings and invalid
known values are rejected rather than replaced with defaults. Unknown launcher JSON,
INI and mod-state fields survive. The interpreter fallback is always retained.

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
