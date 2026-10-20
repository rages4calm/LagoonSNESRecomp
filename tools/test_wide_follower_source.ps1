param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$OutDir=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
  . ../env.ps1
  $rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'
  $identity=@(Select-String -LiteralPath 'rom_identity.txt' -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})"?\s*(?:#.*)?$')
  if($identity.Count -ne 1){throw 'Missing or ambiguous original ROM SHA256'}
  $expected=$identity[0].Matches[0].Groups[1].Value.ToLowerInvariant()
  if((Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected){throw 'Original Lagoon ROM identity mismatch'}
  New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
  $exe=Join-Path $OutDir 'follower-original-source.exe'
  & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_follower_source.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $exe
  if($LASTEXITCODE){throw 'Follower source-oracle compilation failed'}
  & $exe $rom
  if($LASTEXITCODE){throw 'Follower source-oracle failed'}
  $story=Join-Path $OutDir 'story-spawn-source.exe'
  & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_story_spawn_source.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $story
  if($LASTEXITCODE){throw 'Story NPC source-oracle compilation failed'}
  & $story $rom
  if($LASTEXITCODE){throw 'Story NPC source-oracle failed'}
} finally {Pop-Location}
