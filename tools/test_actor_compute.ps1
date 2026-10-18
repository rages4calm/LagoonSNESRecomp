param(
 [Parameter(Mandatory=$true)][string]$Rom,
 [Parameter(Mandatory=$true)][string]$OutDir,
 [string]$Compiler='clang'
)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$romPath=(Resolve-Path -LiteralPath $Rom).Path
$expected=((Get-Content -LiteralPath (Join-Path $repo 'rom_identity.txt') | Where-Object { $_ -match '^expected_sha256\s*=' }) -replace '^expected_sha256\s*=\s*','').Trim()
if ((Get-FileHash -LiteralPath $romPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) { throw 'Actor oracle requires the pinned clean ROM.' }
$target=[IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $target | Out-Null
Push-Location $repo
try {
 $binary=Join-Path $target 'actor-compute.exe'
 & $Compiler -std=c11 -O2 -Isrc -Isnesrecomp/runner/src -Wall -Wextra -Werror -DLAGOON_ACTOR_COMPUTE_TEST tests/lagoon_actor_compute_test.c src/lagoon_actor_compute.c snesrecomp/runner/src/snes/interp816.c -o $binary
 if ($LASTEXITCODE) { throw 'Actor compute oracle compilation failed.' }
 & $binary $romPath
 if ($LASTEXITCODE) { throw 'Actor compute original-opcode oracle failed.' }
} finally { Pop-Location }
