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

Every run gets its own battery-backed card (build\saves\<tag>, handed to the
host as --save-root) because the card is part of the starting state: a run that
inherits another run's saved game measures that run, not the build. -CleanSaves
wipes this tag's cards first, so a fresh-card route really starts fresh; leave
it out and the tag resumes its own last session, which is its own useful test.
Screenshot legs always start from a wiped card of their own — a photograph of
frame N has to begin where the long run began, or it is a different game at
frame N. -Exe points the same route at a different binary, which is how the
ahead-of-time build is checked against build\lagoon-floor.exe, the interpreter
floor reference: two builds that disagree on a replay are not the same port.

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
    # 'dummy' keeps a soak silent and frame-deterministic. 'directsound' opens
    # the real audio device while video stays headless: the audio callback then
    # paces the guest, which is what a player's machine does, so it exposes the
    # timing jitter that makes two untouched runs disagree. Never compare an
    # audio-paced stream against a dummy-audio stream and call the difference a
    # code-generation fault.
    [string]$AudioDriver = 'dummy',
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
# Each run gets its own battery-backed card. The card is part of the starting
# state: a run that inherits another run's saved game is measuring that run, not
# the build. Paths here are relative because the host's working directory is
# build\, which is where the host looks for a relative --save-root.
$saveRoot = 'saves\{0}' -f $Tag
if ($CleanSaves) {
    $saves = Join-Path $build 'saves'
    $mine = @($Tag) + @($Shots | ForEach-Object { "{0}-shot{1}" -f $Tag, $_ })
    if (Test-Path $saves) {
        Get-ChildItem $saves -Directory | Where-Object { $mine -contains $_.Name } |
            Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
    }
}

$env:SDL_VIDEODRIVER = 'dummy'
# Nothing here listens to the SPC unless -AudioDriver says otherwise: the checks
# are framebuffer checksums and scene-state dumps, and the APU keeps emulating
# either way. Dummy audio keeps a 20-minute soak silent *and* deterministic;
# -AudioDriver directsound opens the real device, which paces the guest the way
# a player's machine does.
$env:SDL_AUDIODRIVER = $AudioDriver
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

function Invoke-Host([string]$log, [string]$saveRoot) {
    # Start-Process needs the inner quotes spelled out -- a ROM path with a
    # space otherwise arrives as two arguments and the host opens "Lagoon".
    $argv = @('--no-launcher', '--rom', ('"{0}"' -f $rom), '--save-root', ('"{0}"' -f $saveRoot)) + @($ExeArg | ForEach-Object { '"{0}"' -f $_ })
    if ($Script) { $argv += @('--script', ('"{0}"' -f $Script)) }
    $p = Start-Process -FilePath $exe -ArgumentList $argv -WorkingDirectory $build `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardError $log -RedirectStandardOutput ($log + '.out')
    return $p.ExitCode
}

# 1) One long run that logs every present: the luma curve of the session.
$env:SNESRECOMP_RUN_FRAMES = "$Frames"
$env:SNESRECOMP_PRESENT_LOG = (Join-Path $dir 'presents.csv')
Invoke-Host (Join-Path $dir ("run_{0}.log" -f $Frames)) $saveRoot | Out-Null
Write-Host '== present-luma curve:'
python (Join-Path $PSScriptRoot 'present_curve.py') (Join-Path $dir 'presents.csv')

# 2) One short run per requested frame, so each becomes a PNG.
foreach ($n in $Shots) {
    # A screenshot of frame N has to start from the same card the long run
    # started from, so each leg gets a card of its own, wiped. Inherit the long
    # run's and you can photograph a different game at frame N.
    $shotRoot = 'saves\{0}-shot{1}' -f $Tag, $n
    Remove-Item -Recurse -Force (Join-Path $build ($shotRoot -replace '/', '\')) -ErrorAction SilentlyContinue
    $env:SNESRECOMP_RUN_FRAMES = "$($n + 1)"
    $env:SNESRECOMP_PRESENT_LOG = $null
    $env:SNESRECOMP_SCREENSHOT_FRAME = "$n"
    $env:SNESRECOMP_SCREENSHOT = Join-Path $dir ('present_{0:d6}.ppm' -f $n)
    Invoke-Host (Join-Path $dir ("shot_{0}.log" -f $n)) $shotRoot | Out-Null
}
if ($Shots.Count) {
    Write-Host '== screenshots:'
    python (Join-Path $PSScriptRoot 'ppm_to_png.py') $dir
}
Write-Host "== output: $dir"
