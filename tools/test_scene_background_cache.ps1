param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutDir)
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
  . ../env.ps1
  New-Item -ItemType Directory -Force -Path $out | Out-Null
  $binary=Join-Path $out 'scene-background-cache.exe'
  & $env:CC -std=c11 -O2 -Wall -Wextra -Wno-unused-function -Wno-missing-field-initializers -ffunction-sections -fdata-sections '-Wl,--gc-sections' -Isrc -Isnesrecomp/runner/src tests/lagoon_scene_background_cache_test.c -o $binary
  if($LASTEXITCODE){throw 'Background cache test compile failed'}
  & $binary
  if($LASTEXITCODE){throw 'Background cache test failed'}
} finally {Pop-Location}
