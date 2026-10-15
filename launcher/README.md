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
are window/fullscreen mode, window scale, smoothing, vertical sync, audio enable,
volume, player-one gamepad enable and analog deadzone. Existing keyboard bindings,
button mappings, controller GUIDs and unknown fields are retained. Apply writes only
changed native controls; restart the game to use them. Selecting another executable
or INI and applying loads that source's settings without writing the previous
source's values into it. Make further setting edits after that refresh.

Experimental widescreen defaults off and offers 16:9, 16:10, 21:9, 32:9 and follow
window choices through `[Graphics] Widescreen` and `[Lagoon] WideAspect`. It expands
backgrounds; off-screen NPC/enemy rendering and some rooms remain unfinished.
Unsupported scenes retain their original view.

The Quality of life page changes the matching native package's feature selections
in `mods/preloaded/state.toml` beside the executable. All nine options default off:
walking speed, opening balance, camera centering, sword reach, follower speed,
recovery time, EXP rewards, jump peak hold and magic strength. They require the
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
