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
  $ppuFlags = $decodeFlags + @('-DSNESRECOMP_REVERSE_DEBUG=0','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes')
  Invoke-TestBuild 'wide-entity-pixels' @('tools/test_wide_entity_pixels.c','src/lagoon_wide_entities.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $ppuFlags
  Invoke-TestBuild 'shadow-authoritative-map' @('snesrecomp/tests/ppu/ppu_shadow_authoritative_map_test.c','snesrecomp/runner/src/snes/ws_shadow.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $ppuFlags
  $dmaFlags = @('-DSNESRECOMP_REVERSE_DEBUG=0','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
  Invoke-TestBuild 'dma-start-observer' @('snesrecomp/tests/runtime_dispatch/dma_start_observer_test.c','snesrecomp/runner/src/snes/dma.c') $dmaFlags
} finally { Pop-Location }
