param([Parameter(Mandatory=$true)][string]$ScratchRoot)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$workspaceRoot = Split-Path $repoRoot -Parent
. (Join-Path $workspaceRoot 'env.ps1')
$testRoot = [IO.Path]::GetFullPath($ScratchRoot)
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $workspaceRoot 'work-native-batch49')) + [IO.Path]::DirectorySeparatorChar
if (!$testRoot.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Test root must be a new directory within work-native-batch49.' }
if (Test-Path -LiteralPath $testRoot) { throw 'Use a new synthetic test root; existing inputs are never replaced.' }
New-Item -ItemType Directory -Path $testRoot | Out-Null
$deps = Join-Path $env:RETCOMM_TOOLCHAIN_DIR 'deps'
$executable = Join-Path $testRoot 'achievement-core-test.exe'
$compileArgs = @('-std=c11','-O2','-Wall','-Wextra','-DLAGOON_ACHIEVEMENTS_TEST','-static',
    "-I$repoRoot/src", "-I$repoRoot/third_party/companion_achievements", "-I$repoRoot/third_party/cjson", "-I$deps/include",
    "$repoRoot/tools/test_lagoon_achievements.c", "$repoRoot/src/lagoon_achievements.c",
    "$repoRoot/third_party/companion_achievements/companion_achievements.c", "$repoRoot/third_party/cjson/cJSON.c",
    "$deps/lib/libSDL3.a", '-ldbghelp','-lm','-lopengl32','-lkernel32','-luser32','-lgdi32','-lwinmm','-limm32',
    '-lole32','-loleaut32','-lversion','-luuid','-ladvapi32','-lsetupapi','-lshell32','-ldinput8','-lwinspool','-lcomdlg32',
    '-o', $executable)
& $env:CC @compileArgs
if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $LASTEXITCODE" }
& $executable run (Join-Path $testRoot 'profile')
if ($LASTEXITCODE -ne 0) { throw "Core regression failed: $LASTEXITCODE" }
