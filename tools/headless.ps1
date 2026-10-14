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

.EXAMPLE
.\tools\headless.ps1 -Frames 1800 -Shots 60,300,900 -Tag boot
#>
[CmdletBinding()]
param(
    [int]$Frames = 1800,
    [int[]]$Shots = @(),
    [string]$Tag = 'run',
    [string]$Script = '',
    [string]$DumpDir = '',
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

Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $dir | Out-Null

$env:SDL_VIDEODRIVER = 'dummy'
$env:SNESRECOMP_SCREENSHOT_DIR = $null
$env:SNESRECOMP_SCREENSHOT = $null
$env:SNESRECOMP_SCREENSHOT_FRAME = $null
$env:SNESRECOMP_PRESENT_LOG = $null
if ($DumpDir) {
    Remove-Item -Recurse -Force $DumpDir -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $DumpDir | Out-Null
    $env:SNESRECOMP_DUMP_DIR = $DumpDir
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
