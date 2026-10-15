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
is performed. Display/audio/input and QoL controls are tracked as pending.

`--smoke-test <fresh-directory>` renders the two pages and exercises draft/Revert
behavior without launching the game. `tests/Lagoon.Launcher.Tests.csproj` checks
clean/wrong ROM readiness, structured arguments and an isolated 120-frame native
startup. Its arguments are `native-exe clean-rom fresh-output-directory`.

The shared packages and this adapter are MIT licensed; see LICENSE.
