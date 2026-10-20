param(
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$LoaderSnapshots,
  [switch]$TownOnly,
  [switch]$FieldOnly
)
$ErrorActionPreference='Stop'
if($TownOnly -and $FieldOnly){throw 'Choose one focused family scope'}
# Resolve caller-relative paths before changing the PowerShell location. The
# process working directory used by System.IO can differ from Get-Location.
$callerPath=(Get-Location).Path
$OutDir=if([IO.Path]::IsPathRooted($OutDir)){[IO.Path]::GetFullPath($OutDir)}else{[IO.Path]::GetFullPath((Join-Path $callerPath $OutDir))}
if($LoaderSnapshots){$LoaderSnapshots=if([IO.Path]::IsPathRooted($LoaderSnapshots)){[IO.Path]::GetFullPath($LoaderSnapshots)}else{[IO.Path]::GetFullPath((Join-Path $callerPath $LoaderSnapshots))}}
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
  $sourceArgs=@($rom)
  if($TownOnly){$sourceArgs+='--town-only'}
  if($FieldOnly){$sourceArgs+='--field-only'}
  & $source @sourceArgs
  if($LASTEXITCODE){throw 'Additional actor source-oracle failed'}
  if($TownOnly -or $FieldOnly) {
    $oam=Join-Path $target 'town-original-oam.exe'
    & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_town_oam.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $oam
    if($LASTEXITCODE){throw 'Town original OAM-oracle compilation failed'}
    $oracleArgs=@($rom);if($FieldOnly){$oracleArgs+='--field-only'}
    & $oam @oracleArgs
    if($LASTEXITCODE){throw 'Town original OAM-oracle failed'}
    $animation=Join-Path $target 'town-original-animation.exe'
    & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_town_animation.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $animation
    if($LASTEXITCODE){throw 'Town original animation-oracle compilation failed'}
    & $animation @oracleArgs
    if($LASTEXITCODE){throw 'Town original animation-oracle failed'}
  }
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
    $producerSource=if($TownOnly){'tools/test_wide_town_producer.c'}elseif($FieldOnly){'tools/test_wide_field_producer.c'}else{'tools/test_wide_additional_producer.c'}
    & $env:CC -std=c11 -O2 @flags $producerSource src/lagoon_scene_renderer.c snesrecomp/runner/src/snes/ppu.c snesrecomp/runner/src/snes/ppu_legacy.c -o $producer
    if($LASTEXITCODE){throw 'Additional actor production-policy compilation failed'}
    & $producer $rom $snapshots
    if($LASTEXITCODE){throw 'Additional actor production-policy failed'}
    if($TownOnly -or $FieldOnly) {
      $scene=Join-Path $target 'town-scene-source.exe'
      & $env:CC -std=c11 -O2 -DSNESRECOMP_REVERSE_DEBUG=0 -Isrc -Isnesrecomp/runner/src -Isnesrecomp/runner/src/snes -ffunction-sections -fdata-sections '-Wl,--gc-sections' tests/lagoon_scene_families_test.c -o $scene
      if($LASTEXITCODE){throw 'Town scene source-oracle compilation failed'}
      $scope=if($FieldOnly){'--field-only'}else{'--town-only'}
      & $scene $rom $snapshots 'baseline-' $scope
      if($LASTEXITCODE){throw 'Town scene source-oracle failed'}
    }
  } else {
    Write-Output 'Only original-opcode selector cases executed; no loader snapshots supplied for synthetic production-policy cases.'
  }
} finally {Pop-Location}
