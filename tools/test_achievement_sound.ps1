param([Parameter(Mandatory=$true)][string]$ScratchRoot,[string]$LaughFile)
$ErrorActionPreference='Stop'
$repoRoot=Split-Path $PSScriptRoot -Parent
$workspaceRoot=Split-Path $repoRoot -Parent
. (Join-Path $workspaceRoot 'env.ps1')
$testRoot=[IO.Path]::GetFullPath($ScratchRoot)
$allowedRoots=@('work-native-batch49','work-native-batch50','work-native-batch51') | ForEach-Object { [IO.Path]::GetFullPath((Join-Path $workspaceRoot $_))+[IO.Path]::DirectorySeparatorChar }
if(!($allowedRoots | Where-Object {$testRoot.StartsWith($_,[StringComparison]::OrdinalIgnoreCase)})){throw 'Sound tests require a new scratch directory inside work-native-batch49,50 or51.'}
if(Test-Path -LiteralPath $testRoot){throw 'Existing test inputs are preserved. Choose a new synthetic scratch directory.'}
New-Item -ItemType Directory -Path $testRoot | Out-Null
$deps=Join-Path $env:RETCOMM_TOOLCHAIN_DIR 'deps'
$executable=Join-Path $testRoot 'achievement-sound-test.exe'
$compileArgs=@('-std=c11','-O2','-Wall','-Wextra','-DSNESRECOMP_SDL3=1','-static',"-I$repoRoot/src","-I$repoRoot/snesrecomp/runner/src","-I$deps/include",
    "$repoRoot/tests/lagoon_achievement_sound_test.c","$deps/lib/libSDL3.a",'-ldbghelp','-lm','-lopengl32','-lkernel32','-luser32','-lgdi32','-lwinmm','-limm32',
    '-lole32','-loleaut32','-lversion','-luuid','-ladvapi32','-lsetupapi','-lshell32','-ldinput8','-lwinspool','-lcomdlg32','-o',$executable)
& $env:CC @compileArgs
if($LASTEXITCODE -ne 0){throw "Compilation failed: $LASTEXITCODE"}
if(!$LaughFile){$LaughFile=Join-Path $repoRoot 'assets/easter_egg/B1.wav'}
if(!(Test-Path -LiteralPath $LaughFile)){throw 'Supply the optional laugh WAV with -LaughFile.'}
foreach($device in @('dummy','actual')){
    & $executable $device (Join-Path $testRoot "$device.json") ([IO.Path]::GetFullPath($LaughFile))
    if($LASTEXITCODE -ne 0){throw "Sound $device regression failed: $LASTEXITCODE"}
}
