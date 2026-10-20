param(
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$LoaderSnapshots
)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
  . ../env.ps1
  $rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'
  $identity=@(Select-String -LiteralPath (Join-Path $repo 'rom_identity.txt') -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})"?\s*(?:#.*)?$')
  if($identity.Count -ne 1){throw 'Missing or ambiguous expected_sha256 in rom_identity.txt'}
  $expected=$identity[0].Matches[0].Groups[1].Value.ToLowerInvariant()
  $actual=(Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant()
  if($actual -ne $expected){throw 'Additional actor tests require the verified original Lagoon ROM from rom_identity.txt'}
  $target=[IO.Path]::GetFullPath($OutDir)
  New-Item -ItemType Directory -Force -Path $target | Out-Null
  $source=Join-Path $target 'additional-actor-source.exe'
  & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_additional_actors.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $source
  if($LASTEXITCODE){throw 'Additional actor source-oracle compilation failed'}
  & $source $rom
  if($LASTEXITCODE){throw 'Additional actor source-oracle failed'}
  if($LoaderSnapshots) {
    # Requires fresh actual loader dumps at baseline-2c/2f/35/dumps/sample-11.*.
    # Policy actor/OAM placement is synthetic; original spawned actors require
    # the separate bounded runtime off/auto routes documented by the handoff.
    $snapshots=[IO.Path]::GetFullPath($LoaderSnapshots)
    $flags=@('-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0',
      '-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes',
      '-Isnesrecomp/runner/src/desktop',"-I$env:RETCOMM_TOOLCHAIN_DIR/deps/include",
      '-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
    $producer=Join-Path $target 'additional-actor-producer.exe'
    & $env:CC -std=c11 -O2 @flags tools/test_wide_additional_producer.c src/lagoon_scene_renderer.c snesrecomp/runner/src/snes/ppu.c snesrecomp/runner/src/snes/ppu_legacy.c -o $producer
    if($LASTEXITCODE){throw 'Additional actor production-policy compilation failed'}
    & $producer $rom $snapshots
    if($LASTEXITCODE){throw 'Additional actor production-policy failed'}
  } else {
    Write-Output 'Only original-opcode selector cases executed; no loader snapshots supplied for synthetic production-policy cases.'
  }
} finally {Pop-Location}
