param(
  [Parameter(Mandatory=$true)][string]$Rom,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$Compiler='clang'
)
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$romPath=(Resolve-Path -LiteralPath $Rom).Path
$expected=((Get-Content -LiteralPath (Join-Path $repo 'rom_identity.txt') | Where-Object {$_ -match '^expected_sha256\s*='}) -replace '^expected_sha256\s*=\s*','').Trim()
if((Get-FileHash -LiteralPath $romPath).Hash -ne $expected){throw 'Requires the pinned clean ROM'}
$target=[IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $target | Out-Null
Push-Location $repo
try {
  $exe=Join-Path $target 'native-recovery-guard.exe'
  & $Compiler -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Isnesrecomp/runner/src tests/lagoon_native_recovery_guard_test.c src/lagoon_native_recovery.c -o $exe
  if($LASTEXITCODE){throw 'Recovery guard compilation failed'}
  & $exe $romPath
  if($LASTEXITCODE){throw 'Recovery guard test failed'}

  $boss=Join-Path $target 'native-boss-continuation.exe'
  & $Compiler -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Isnesrecomp/runner/src tests/lagoon_boss_continuation_test.c -o $boss
  if($LASTEXITCODE){throw 'Boss continuation guard compilation failed'}
  & $boss $romPath
  if($LASTEXITCODE){throw 'Boss continuation guard test failed'}

  $flash=Join-Path $target 'native-death-flash.exe'
  & $Compiler -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Isnesrecomp/runner/src tests/lagoon_death_flash_guard_test.c -o $flash
  if($LASTEXITCODE){throw 'Death flash guard compilation failed'}
  & $flash $romPath
  if($LASTEXITCODE){throw 'Death flash guard test failed'}

  # Compile the current production matcher verbatim; no SDK edits or copy tree.
  $header=Get-Content -LiteralPath 'snesrecomp/runner/src/snes/interp_bridge.h' -Raw
  $source=Get-Content -LiteralPath 'snesrecomp/runner/src/snes/interp_bridge.c' -Raw
  $types=($header -split '/\* BEGIN AOT_CONTINUATION_TYPES \*/',2)[1] -split '/\* END AOT_CONTINUATION_TYPES \*/',2
  $policy=($source -split '/\* BEGIN AOT_CONTINUATION_POLICY -- pure, bounded provenance matcher\. \*/',2)[1] -split '/\* END AOT_CONTINUATION_POLICY \*/',2
  if(-not $types[0] -or -not $policy[0]){throw 'Production matcher markers missing'}
  ($types[0]+$policy[0]) | Set-Content -LiteralPath (Join-Path $target 'continuation_policy_under_test.h') -Encoding UTF8
  $token=Join-Path $target 'native-death-token.exe'
  & $Compiler -std=c11 -O2 -Wall -Wextra -Werror "-I$target" tests/lagoon_native_death_token_test.c -o $token
  if($LASTEXITCODE){throw 'Death token matcher compilation failed'}
  & $token
  if($LASTEXITCODE){throw 'Death token matcher test failed'}
} finally {Pop-Location}
