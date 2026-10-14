<#
.SYNOPSIS
Replay-parity gate: run one route on the ahead-of-time build and on the
interpreter floor, then diff the replays byte for byte.

.DESCRIPTION
The promotion loop's last step is a replay comparison, not a screenshot. This
runs the same pad script through both binaries from a wiped save card, then
diffs the per-present checksum stream and every framedump component
(framebuffer, WRAM, VRAM, CGRAM, OAM, register snapshot). Anything that is not
identical is printed with the frame it first appeared at, because a promotion
that changes the replay is not a promotion — it is a bug you have not localized
yet.

Exit code is 0 only when both streams agree everywhere.

Build the floor reference once with:
  tools\regen.sh --cfg-dir recomp-floor   (then copy build\LagoonSNESRecomp.exe
  to build\lagoon-floor.exe)

.EXAMPLE
.\tools\replay_parity.ps1 -Script tools\script_move.txt -Frames 2760 -Dumps
.\tools\replay_parity.ps1 -Script tools\script_intro.txt -Frames 12000 -Shots 3000,6000
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Script,
    [int]$Frames = 2760,
    [int[]]$Shots = @(),
    [switch]$Dumps,
    [string]$FloorExe = 'build\lagoon-floor.exe',
    [string]$AotExe = 'build\LagoonSNESRecomp.exe',
    [string]$Tag = 'parity'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

$common = @{ Frames = $Frames; Shots = $Shots; Script = $Script; CleanSaves = $true }
foreach ($side in @(@{ n = 'floor'; exe = $FloorExe }, @{ n = 'aot'; exe = $AotExe })) {
    $args = @{} + $common
    $args['Tag'] = "$Tag-$($side.n)"
    $args['Exe'] = $side.exe
    if ($Dumps) { $args['DumpDir'] = "build\dumps-$Tag-$($side.n)" }
    Write-Host "== $($side.n): $($side.exe)"
    & (Join-Path $PSScriptRoot 'headless.ps1') @args
}

$left = Join-Path $repo "build\shots-$Tag-floor\presents.csv"
$right = Join-Path $repo "build\shots-$Tag-aot\presents.csv"
$a = Get-Content $left
$b = Get-Content $right
$n = [Math]::Min($a.Count, $b.Count)
$diff = 0
$first = -1
for ($i = 1; $i -lt $n; $i++) {
    if ($a[$i] -ne $b[$i]) {
        $diff++
        if ($first -lt 0) {
            $first = $i
            Write-Host "first divergence at frame $($i + 1):"
            Write-Host "  floor: $($a[$i])"
            Write-Host "  aot  : $($b[$i])"
        }
    }
}
Write-Host "== present stream: $n frames compared, $diff differing"
$bad = $diff
if ($a.Count -ne $b.Count) {
    Write-Host "== length mismatch: floor=$($a.Count - 1), aot=$($b.Count - 1) presents"
    $bad += 1
}
if ($n -lt 2) {
    Write-Host '== no presented frames were compared'
    $bad += 1
}
if ($Dumps) {
    python (Join-Path $PSScriptRoot 'dump_compare.py') (Join-Path $repo "build\dumps-$Tag-aot") (Join-Path $repo "build\dumps-$Tag-floor")
    # 0 = identical, 1 = something differs, 3 = this route has no `dump` lines
    # at all, which is a note about the script, not a failed check.
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 3) { $bad += 1 }
    elseif ($LASTEXITCODE -eq 3) { Write-Host '== no framedumps on this route (add `dump <tag>` lines to the script)' }
}
if ($bad -eq 0) {
    Write-Host "== PARITY: the ahead-of-time build replays the route exactly like the floor"
    exit 0
}
Write-Host "== NOT AT PARITY ($bad mismatching checks)"
exit 1
