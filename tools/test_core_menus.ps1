param(
  [Parameter(Mandatory=$true)][string]$Rom,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$Compiler = 'clang'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$romPath = (Resolve-Path -LiteralPath $Rom).Path
$target = [IO.Path]::GetFullPath($OutDir)
$identity = Get-Content -LiteralPath (Join-Path $repo 'rom_identity.txt')
$expected = ($identity | Where-Object { $_ -match '^expected_sha256\s*=' }) -replace '^expected_sha256\s*=\s*', ''
if (-not $expected -or (Get-FileHash -LiteralPath $romPath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected.Trim()) {
  throw 'Core menu oracle requires the clean ROM pinned in rom_identity.txt.'
}
New-Item -ItemType Directory -Force -Path $target | Out-Null
Push-Location $repo
try {
  $flags = @('-std=c11', '-O2', '-Isrc', '-Isnesrecomp/runner/src')
  $fadeUnit = Join-Path $target 'menu-fade-unit.exe'
  & $Compiler @flags -Wall -Wextra -Werror -DLAGOON_FAST_MENUS_TEST tests/lagoon_fast_menus_test.c src/lagoon_fast_menus.c -o $fadeUnit
  if ($LASTEXITCODE) { throw 'Menu fade unit compilation failed.' }
  & $fadeUnit
  if ($LASTEXITCODE) { throw 'Menu fade unit test failed.' }

  $fadeOracle = Join-Path $target 'menu-fade-original-oracle.exe'
  & $Compiler @flags -Wall -Wextra -Werror -DLAGOON_FAST_MENUS_TEST tests/lagoon_fast_menus_wait_oracle_test.c src/lagoon_fast_menus.c snesrecomp/runner/src/snes/interp816.c -o $fadeOracle
  if ($LASTEXITCODE) { throw 'Menu fade original-opcode oracle compilation failed.' }
  & $fadeOracle $romPath
  if ($LASTEXITCODE) { throw 'Menu fade original-opcode oracle failed.' }
  $decode = 'src/lagoon_menu_decode.c'
  $unit = Join-Path $target 'menu-decode-unit.exe'
  & $Compiler @flags -DLAGOON_MENU_DECODE_TEST tests/lagoon_menu_decode_test.c $decode -o $unit
  if ($LASTEXITCODE) { throw 'Menu decoder unit compilation failed.' }
  & $unit
  if ($LASTEXITCODE) { throw 'Menu decoder unit test failed.' }

  $guard = Join-Path $target 'menu-decode-guard.exe'
  & $Compiler @flags tests/lagoon_menu_decode_guard_test.c $decode -o $guard
  if ($LASTEXITCODE) { throw 'Menu decoder guard compilation failed.' }
  & $guard $romPath
  if ($LASTEXITCODE) { throw 'Menu decoder guard test failed.' }

  $oracle = Join-Path $target 'menu-decode-original-oracle.exe'
  & $Compiler @flags -DLAGOON_MENU_DECODE_TEST tests/lagoon_menu_decode_oracle_test.c $decode snesrecomp/runner/src/snes/interp816.c -o $oracle
  if ($LASTEXITCODE) { throw 'Original-opcode oracle compilation failed.' }
  & $oracle $romPath
  if ($LASTEXITCODE) { throw 'Original-opcode oracle failed.' }

  $closeOracle = Join-Path $target 'menu-close-original-oracle.exe'
  & $Compiler @flags -Wall -Wextra -Werror -DLAGOON_MENU_CLOSE_TEST tests/lagoon_menu_close_oracle_test.c src/lagoon_menu_close.c snesrecomp/runner/src/snes/interp816.c -o $closeOracle
  if ($LASTEXITCODE) { throw 'Menu close original-opcode oracle compilation failed.' }
  & $closeOracle $romPath
  if ($LASTEXITCODE) { throw 'Menu close original-opcode oracle failed.' }

  $prefixOracle = Join-Path $target 'menu-close-prefix-original-oracle.exe'
  & $Compiler @flags -Wall -Wextra -Werror -DLAGOON_MENU_CLOSE_TEST tests/lagoon_menu_close_prefix_oracle_test.c src/lagoon_menu_close.c snesrecomp/runner/src/snes/interp816.c -o $prefixOracle
  if ($LASTEXITCODE) { throw 'Menu close parent-prefix oracle compilation failed.' }
  & $prefixOracle $romPath
  if ($LASTEXITCODE) { throw 'Menu close parent-prefix original-opcode oracle failed.' }

  $closeGuard = Join-Path $target 'menu-close-production-guard.exe'
  & $Compiler @flags -Wall -Wextra -Werror tests/lagoon_menu_close_guard_test.c src/lagoon_menu_close.c -o $closeGuard
  if ($LASTEXITCODE) { throw 'Menu close production guard compilation failed.' }
  & $closeGuard $romPath
  if ($LASTEXITCODE) { throw 'Menu close production guard test failed.' }
} finally { Pop-Location }
