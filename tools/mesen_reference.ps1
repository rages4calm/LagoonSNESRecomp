<#
.SYNOPSIS
Runs a route through the Mesen core (an independent SNES emulator) and dumps
the same state the port dumps, so a question about the port can be answered
against a second implementation.

.DESCRIPTION
The port can prove two things about itself: that its ahead-of-time build and its
interpreter floor agree, and that a route renders the same frames it rendered
last time. Neither of those can answer "is this what the game does", because
both sides of the comparison are this codebase. A reference core can: same ROM,
same input, same frame count, and an entirely separate implementation of the
hardware.

Input is translated from the port's own route files (tools/script_*.txt), so the
comparison is the route the port was qualified on rather than a second script
that can drift from it. The cost model is the port's: `wait N` costs N frames,
`press b N` costs N+1 (the release edge), `dump` and `quit` cost nothing.

The agent prints JSON Lines on stdout; the screenshot is the last rendered frame,
so a run ends at the frame worth looking at. WRAM comes out of Mesen as a flat
128 KiB blob and compares byte for byte against <tag>.wram.bin from the port.

Tool provenance: Mesen Agent came from the EarthBound project's tools directory
(see C:\Users\crono\Desktop\Projects\eb\tools\mesen-agent-v0.1.0); MesenCore is a
standalone emulator core, not this project's code.

.EXAMPLE
.\tools\mesen_reference.ps1 -Script tools\script_move.txt -Frames 2760 -Tag mv
.\tools\mesen_reference.ps1 -Script tools\script_move.txt -Frames 500 -Tag boot500 -ButtonProbe
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Script,
    [int]$Frames = 600,
    [string]$Tag = 'mesen',
    [string]$Agent = 'C:\Users\crono\Desktop\Projects\eb\tools\mesen-agent-v0.1.0\mesen-agent.exe',
    # Stop after this many input overrides; 0 uses them all. Lets a short run
    # reproduce the first N seconds of a long route exactly.
    [int]$MaxInputs = 0,
    # The agent's own default is 30 seconds of wall clock, which is about 1 800
    # frames -- it stops early and reports ok:false, reason:timeout rather than
    # failing, so a long route silently compares two unrelated moments.
    [int]$Timeout = 3600,
    [switch]$ButtonProbe
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$rom = if ($env:SNESRECOMP_ROM) { $env:SNESRECOMP_ROM }
       else { Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc' }
$scriptPath = if ([IO.Path]::IsPathRooted($Script)) { $Script } else { Join-Path $repo $Script }
$outDir = Join-Path $repo ("build\mesen-{0}" -f $Tag)
Remove-Item -Recurse -Force $outDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
# Per-tag so two runs can go at once: Mesen keeps its battery saves and its
# firmware cache in here, and two processes writing one directory is a race
# over a file neither of them expects to move.
$home0 = Join-Path $repo ("build\mesen-home-{0}" -f $Tag)
New-Item -ItemType Directory -Force -Path $home0 | Out-Null

# Route -> Mesen's --input overrides. Mesen wants PORT:BUTTONS@START-END in
# absolute frames; the port's script is a stream of costs, so walk it.
$kMesenName = @{ a='A'; b='B'; x='X'; y='Y'; l='L'; r='R'; up='Up'; down='Down';
                 left='Left'; right='Right'; start='Start'; select='Select' }
$args0 = @()
$frame = 0
$held = New-Object System.Collections.Generic.List[string]
foreach ($line in Get-Content $scriptPath) {
    $t = $line.Trim()
    if (-not $t -or $t.StartsWith('#')) { continue }
    $p = $t -split '\s+'
    switch ($p[0]) {
        'wait'  { $frame += [int]$p[1] }
        'press' {
            $btn = $p[1].ToLower(); $n = [int]$p[2]
            $name = $kMesenName[$btn]
            if (-not $name) { throw "route uses unmapped button '$btn'" }
            $held.Add("1:$name@$frame-$($frame + $n - 1)")
            $frame += $n + 1
        }
        default { }   # dump / quit: no frame cost, no input
    }
}
$use = if ($MaxInputs -gt 0) { $held[0..([Math]::Min($MaxInputs, $held.Count) - 1)] } else { $held }
$argv = @($rom, '--frames', "$Frames", '--home', $home0, '--timeout', "$Timeout")
foreach ($h in $use) { $argv += '--input'; $argv += $h }
$shot = Join-Path $outDir 'last.png'
$argv += @('--screenshot', $shot,
           '--dump-memory', "SnesWorkRam=$outDir\wram.bin",
           '--dump-memory', "SnesVideoRam=$outDir\vram.bin",
           '--dump-memory', "SnesCgRam=$outDir\cgram.bin",
           '--dump-memory', "SnesSpriteRam=$outDir\oam.bin",
           '--log', (Join-Path $outDir 'mesen.log'))
if ($ButtonProbe) { $argv = @($rom, '--frames', '120', '--home', $home0, '--input', '1:A@50-60', '--pc') }

"== mesen: $([Math]::Max(0,$Frames)) frames, $($use.Count) input overrides (route is $frame frames of commands)"
$sw = [Diagnostics.Stopwatch]::StartNew()
# The agent speaks JSON Lines; keep both streams so a refusal is readable.
$stdout = & $Agent @argv 2>&1
$rc = $LASTEXITCODE
$sw.Stop()
$stdout | Set-Content -Encoding ascii -Path (Join-Path $outDir 'stdout.txt')
"== exit $rc in $([int]$sw.Elapsed.TotalSeconds) s -> $outDir"
$stdout | Select-Object -Last 4 | ForEach-Object { "  $_" }
if (Test-Path $shot) { "  screenshot: $shot" }
if (Test-Path "$outDir\wram.bin") { "  wram: $((Get-Item "$outDir\wram.bin").Length) bytes" }
