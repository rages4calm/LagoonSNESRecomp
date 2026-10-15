param([string]$CompanionRoot = '')
$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path $PSScriptRoot -Parent
if (-not $CompanionRoot) {
    $SearchRoot = Get-Item $RepoRoot
    while ($SearchRoot -and -not $CompanionRoot) {
        $Candidate = Join-Path $SearchRoot.FullName 'CompanionLauncher'
        if (Test-Path (Join-Path $Candidate 'packages\Companion.Launcher.Core.0.2.0.nupkg')) { $CompanionRoot = $Candidate }
        $SearchRoot = $SearchRoot.Parent
    }
}
if (-not $CompanionRoot) { throw 'Pass -CompanionRoot with your CompanionLauncher checkout (built packages 0.2.0 required).' }
$env:COMPANION_LAUNCHER_FEED = Join-Path (Resolve-Path $CompanionRoot).Path 'packages'
& dotnet restore (Join-Path $RepoRoot 'launcher\Game.Companion.csproj') --locked-mode
if ($LASTEXITCODE -ne 0) { throw 'Launcher package restore failed' }
& dotnet build (Join-Path $RepoRoot 'launcher\Game.Companion.csproj') -c Release --no-restore --nologo
if ($LASTEXITCODE -ne 0) { throw 'Launcher build failed' }
