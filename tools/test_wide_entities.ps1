param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
  . ../env.ps1
  $target = [IO.Path]::GetFullPath($OutDir)
  New-Item -ItemType Directory -Force -Path $target | Out-Null
  $cc = $env:CC
  function Invoke-TestBuild([string]$Name, [string[]]$Sources, [string[]]$Flags) {
    $exe = Join-Path $target ($Name + '.exe')
    & $cc -std=c11 -O2 @Flags @Sources -o $exe
    if ($LASTEXITCODE) { throw "$Name compilation failed" }
    & $exe
    if ($LASTEXITCODE) { throw "$Name failed" }
  }
  $decodeFlags = @('-DLAGOON_WIDE_ENTITIES_DECODER_ONLY', '-Isrc')
  Invoke-TestBuild 'wide-entity-decoder' @('tools/test_wide_entity_decoder.c','src/lagoon_wide_entities.c') $decodeFlags
  Invoke-TestBuild 'wide-entity-state' @('tools/test_wide_entity_state.c','src/lagoon_wide_entities.c') $decodeFlags
  $parallaxFlags = $decodeFlags + @('-DLAGOON_MENU_DECODE_TEST','-Isnesrecomp/runner/src')
  $parallaxExe = Join-Path $target 'wide-parallax.exe'
  & $cc -std=c11 -O2 @parallaxFlags tools/test_wide_parallax.c src/lagoon_wide_entities.c src/lagoon_menu_decode.c snesrecomp/runner/src/snes/interp816.c -o $parallaxExe
  if ($LASTEXITCODE) { throw 'Parallax source oracle compilation failed' }
  & $parallaxExe (Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc')
  if ($LASTEXITCODE) { throw 'Parallax source oracle failed' }
  $waterExe = Join-Path $target 'wide-water.exe'
  & $cc -std=c11 -O2 @decodeFlags tools/test_wide_water.c src/lagoon_wide_entities.c -o $waterExe
  if ($LASTEXITCODE) { throw 'Water source oracle compilation failed' }
  & $waterExe (Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc')
  if ($LASTEXITCODE) { throw 'Water source oracle failed' }
  $ppuFlags = $decodeFlags + @('-DSNESRECOMP_REVERSE_DEBUG=0','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes')
  Invoke-TestBuild 'wide-entity-pixels' @('tools/test_wide_entity_pixels.c','src/lagoon_wide_entities.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $ppuFlags
  Invoke-TestBuild 'shadow-authoritative-map' @('snesrecomp/tests/ppu/ppu_shadow_authoritative_map_test.c','snesrecomp/runner/src/snes/ws_shadow.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $ppuFlags
  $retainedFlags = @('-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-Isnesrecomp/runner/src/desktop',"-I$env:RETCOMM_TOOLCHAIN_DIR/deps/include",'-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
  Invoke-TestBuild 'wide-retained-objects' @('tools/test_wide_retained_objects.c','src/lagoon_wide_entities.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $retainedFlags
  $dmaFlags = @('-DSNESRECOMP_REVERSE_DEBUG=0','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
  Invoke-TestBuild 'dma-start-observer' @('snesrecomp/tests/runtime_dispatch/dma_start_observer_test.c','snesrecomp/runner/src/snes/dma.c') $dmaFlags
} finally { Pop-Location }
