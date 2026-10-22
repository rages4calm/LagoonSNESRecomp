param(
  [Parameter(Mandatory=$true)][string]$Rom,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$Compiler = 'clang'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$romPath = (Resolve-Path -LiteralPath $Rom).Path
$expected = '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
if ((Get-FileHash -LiteralPath $romPath).Hash.ToLowerInvariant() -ne $expected) { throw 'Use the clean external USA ROM.' }
$output = [IO.Path]::GetFullPath($OutDir)
if (Test-Path -LiteralPath $output) { throw 'Use a fresh test output directory.' }
New-Item -ItemType Directory -Path $output | Out-Null
Push-Location $repo
try {
  $flags = @('-std=c11','-O2','-Isrc','-Isnesrecomp/runner/src')
  $cases = @(
    @{Name='fade'; Sources=@('tests/lagoon_door_fade_test.c','src/lagoon_fast_menus.c'); Extra=@('-DLAGOON_FAST_MENUS_TEST')},
    @{Name='decode'; Sources=@('tests/lagoon_door_decode_guard_test.c','src/lagoon_menu_decode.c'); Extra=@()},
    @{Name='close'; Sources=@('tests/lagoon_door_close_guard_test.c','src/lagoon_menu_close.c'); Extra=@()},
    @{Name='dma'; Sources=@('tests/lagoon_door_dma_test.c','src/lagoon_menu_dma.c','snesrecomp/runner/src/snes/interp816.c'); Extra=@('-DLAGOON_MENU_DMA_TEST')}
  )
  foreach ($case in $cases) {
    $exe = Join-Path $output ($case.Name+'.exe')
    & $Compiler @flags @($case.Extra) @($case.Sources) -o $exe
    if ($LASTEXITCODE) { throw "Door $($case.Name) compilation failed." }
    & $exe $romPath
    if ($LASTEXITCODE) { throw "Door $($case.Name) check failed." }
  }
} finally { Pop-Location }
