<#
.SYNOPSIS
Headless run of the Lagoon port: fixed frame counts, per-present luma log,
PNG screenshots of chosen frames.

.DESCRIPTION
Runs build\LagoonSNESRecomp.exe with SDL_VIDEODRIVER=dummy so no window is
needed. Pass 1 runs the whole span and logs every presented frame's checksum
and mean luma (a black screen becomes a number, not an impression); then one
short run per requested frame, because the host writes a single screenshot
per run, each converted to PNG.

Pass a -Script to drive the pad (see the host's script contract: wait /
press / poke / until / dump / quit). Pass -DumpDir with -Script to get
scene-state dumps (wram/vram/cgram/oam/ppu regs) instead of pictures.

-CleanSaves wipes the battery-backed card (build\saves\save.srm) before the
run. Lagoon reads its save at boot, so every run inherits whatever the last run
saved; a "regression" measured against a baseline taken with different SRAM in
the slot is usually just a different saved game. -Exe points the same route at a
different binary — the ahead-of-time build against build\lagoon-floor.exe, the
interpreter floor reference. Two builds that disagree on a replay are not the
same port; two builds that agree to the byte are.

.EXAMPLE
.\tools\headless.ps1 -Frames 1800 -Shots 60,300,900 -Tag boot
.EXAMPLE
.\tools\headless.ps1 -Frames 2760 -Script tools\script_move.txt -Tag aot -CleanSaves -DumpDir build\dumps-aot
.\tools\headless.ps1 -Frames 2760 -Script tools\script_move.txt -Tag floor -CleanSaves -DumpDir build\dumps-floor -Exe build\lagoon-floor.exe
#>
[CmdletBinding()]
param(
    [int]$Frames = 1800,
    [int[]]$Shots = @(),
    [string]$Tag = 'run',
    [string]$Script = '',
    [string]$DumpDir = '',
    [switch]$Coverage,
    [switch]$CleanSaves,
    [string]$Exe = '',
    [string[]]$ExeArg = @()
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$build = Join-Path $repo 'build'
# The ROM is copyrighted and lives outside the repo: env.ps1 at the workspace
# root exports SNESRECOMP_ROM, which is where this looks first.
$rom = if ($env:SNESRECOMP_ROM) { $env:SNESRECOMP_ROM }
       else { Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc' }
$exe = Join-Path $build 'LagoonSNESRecomp.exe'
$dir = Join-Path $build ("shots-{0}" -f $Tag)

function Resolve-RepoPath([string]$p) {
    # The host runs with -WorkingDirectory build, so a path the caller typed
    # relative to the repo (tools\script_move.txt, build\dumps-move) would
    # otherwise resolve against build\ and silently miss: no script means no
    # `press` lines and no `dump` lines, i.e. a passive player and no dumps,
    # which looks like a game that simply does not react.
    if ([IO.Path]::IsPathRooted($p)) { return $p }
    return (Join-Path $repo $p)
}
if ($Script) { $Script = Resolve-RepoPath $Script }
if ($DumpDir) { $DumpDir = Resolve-RepoPath $DumpDir }
# -Exe compares two builds on one route: the AOT build and the interpreter
# floor reference (build\lagoon-floor.exe) must produce byte-identical
# replays, and a promotion that changes the replay is not a promotion.
if ($Exe) { $exe = Resolve-RepoPath $Exe }

Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $dir | Out-Null
if ($CleanSaves) {
    # The save card is part of the starting state, so it belongs to the run.
    Get-ChildItem (Join-Path $build 'saves') -Filter '*.sr*' -ErrorAction SilentlyContinue |
        Remove-Item -Force
}

$env:SDL_VIDEODRIVER = 'dummy'
# Nothing here listens to the SPC: the checks are framebuffer checksums and
# scene-state dumps, and the APU keeps emulating either way. Dummy audio just
# stops a 20-minute headless soak from playing Lagoon's soundtrack at you.
$env:SDL_AUDIODRIVER = 'dummy'
$env:SNESRECOMP_SCREENSHOT_DIR = $null
$env:SNESRECOMP_SCREENSHOT = $null
$env:SNESRECOMP_SCREENSHOT_FRAME = $null
$env:SNESRECOMP_PRESENT_LOG = $null
if ($DumpDir) {
    Remove-Item -Recurse -Force $DumpDir -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $DumpDir | Out-Null
    $env:SNESRECOMP_DUMP_DIR = $DumpDir
}
if ($Coverage) {
    # Where the interpreter actually ran, why a transfer fell back, and which
    # entry variants are worth analyzing: feed the result to
    #   tools/regen.sh --cfg-roots --profile <manifest>
    # to promote it into ahead-of-time code. See snesrecomp/docs/COVERAGE_FEEDBACK.md.
    $cov = Join-Path $build ("coverage-{0}" -f $Tag)
    Remove-Item -Recurse -Force $cov -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $cov | Out-Null
    $env:SNESRECOMP_TIER2_CAPTURE = '1'
    $env:SNESRECOMP_COVERAGE_DIR = $cov
    Write-Host "== coverage capture -> $cov"
}

function Invoke-Host([string]$log) {
    # Start-Process needs the inner quotes spelled out -- a ROM path with a
    # space otherwise arrives as two arguments and the host opens "Lagoon".
    $argv = @('--no-launcher', '--rom', ('"{0}"' -f $rom)) + @($ExeArg | ForEach-Object { '"{0}"' -f $_ })
    if ($Script) { $argv += @('--script', ('"{0}"' -f $Script)) }
    $p = Start-Process -FilePath $exe -ArgumentList $argv -WorkingDirectory $build `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardError $log -RedirectStandardOutput ($log + '.out')
    return $p.ExitCode
}

# 1) One long run that logs every present: the luma curve of the session.
$env:SNESRECOMP_RUN_FRAMES = "$Frames"
$env:SNESRECOMP_PRESENT_LOG = (Join-Path $dir 'presents.csv')
Invoke-Host (Join-Path $dir ("run_{0}.log" -f $Frames)) | Out-Null
Write-Host '== present-luma curve:'
python (Join-Path $PSScriptRoot 'present_curve.py') (Join-Path $dir 'presents.csv')

# 2) One short run per requested frame, so each becomes a PNG.
foreach ($n in $Shots) {
    $env:SNESRECOMP_RUN_FRAMES = "$($n + 1)"
    $env:SNESRECOMP_PRESENT_LOG = $null
    $env:SNESRECOMP_SCREENSHOT_FRAME = "$n"
    $env:SNESRECOMP_SCREENSHOT = Join-Path $dir ('present_{0:d6}.ppm' -f $n)
    Invoke-Host (Join-Path $dir ("shot_{0}.log" -f $n)) | Out-Null
}
if ($Shots.Count) {
    Write-Host '== screenshots:'
    python (Join-Path $PSScriptRoot 'ppm_to_png.py') $dir
}
Write-Host "== output: $dir"
