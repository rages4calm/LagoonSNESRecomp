param(
 [Parameter(Mandatory=$true)][string]$OutDir,
 [Parameter(Mandatory=$true)][string]$LoaderDumpPrefix,
 [string]$Rom='',
 [string]$Compiler='',
 [string]$SDLInclude=''
)
$ErrorActionPreference='Stop'
$caller=(Get-Location).Path
$OutDir=[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($OutDir)){$OutDir}else{Join-Path $caller $OutDir}))
$LoaderDumpPrefix=[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($LoaderDumpPrefix)){$LoaderDumpPrefix}else{Join-Path $caller $LoaderDumpPrefix}))
if($Rom){$Rom=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Rom)}
if($SDLInclude){$SDLInclude=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($SDLInclude)}
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
 $localEnvironment=Join-Path (Split-Path -Parent $repo) 'env.ps1'
 if(Test-Path -LiteralPath $localEnvironment){. $localEnvironment}
 if(-not $Compiler){$Compiler=if($env:CC){$env:CC}else{'clang'}}
 if(-not (Get-Command $Compiler -ErrorAction SilentlyContinue)){throw 'Set -Compiler or CC to a C11 compiler (for example clang)'}
 if(-not $SDLInclude -and $env:RETCOMM_TOOLCHAIN_DIR){$SDLInclude=Join-Path $env:RETCOMM_TOOLCHAIN_DIR 'deps/include'}
 if(-not $SDLInclude -and $env:SDL3_INCLUDE_DIR){$SDLInclude=$env:SDL3_INCLUDE_DIR}
 if($SDLInclude -and -not (Test-Path -LiteralPath (Join-Path $SDLInclude 'SDL3/SDL.h'))){throw 'SDL include directory must contain SDL3/SDL.h; pass -SDLInclude'}
 if(-not $Rom){$Rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'}
 $identity=@(Select-String -LiteralPath 'rom_identity.txt' -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})"?\s*(?:#.*)?$')
 if($identity.Count -ne 1 -or (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant() -ne $identity[0].Matches[0].Groups[1].Value.ToLowerInvariant()){throw 'Tests require the original ROM pinned in rom_identity.txt'}
 New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
 $runs=@()
 function Invoke-Checked([string]$Name,[string[]]$Sources,[string[]]$Flags,[string[]]$Arguments){
  $exe=Join-Path $OutDir ($Name+'.exe')
  & $Compiler -std=c11 -O2 @Flags @Sources -o $exe
  if($LASTEXITCODE){throw ($Name+' compile failed')}
  $stdout=@(& $exe @Arguments)
  if($LASTEXITCODE){throw ($Name+' failed')}
  $stdout | ForEach-Object {Write-Host $_}
  $script:runs+=@{name=$Name;stdout=$stdout;executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant();sources=$Sources;flags=$Flags;arguments=$Arguments}
 }
 $pure=@('-DLAGOON_WIDE_ENTITIES_DECODER_ONLY','-Isrc','-Isnesrecomp/runner/src')
 Invoke-Checked 'magic-source' @('tools/test_wide_magic_source.c','snesrecomp/runner/src/snes/interp816.c') $pure @($rom)
 $flags=@('-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-Isnesrecomp/runner/src/desktop','-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
 if($SDLInclude){$flags+=('-I'+$SDLInclude)}
 Invoke-Checked 'magic-producer' @('tools/test_wide_magic_producer.c','snesrecomp/runner/src/snes/interp816.c','src/lagoon_wide_multipart.c','src/lagoon_wide_spell3.c','src/lagoon_scene_renderer.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $flags @($rom,$LoaderDumpPrefix)
 $paths=@('tools/test_wide_magic.ps1','tools/test_wide_magic_source.c','tools/test_wide_magic_producer.c','tools/test_wide_parallax_sources.c','tools/test_wide_projectile_producer.c','tools/test_wide_retained_objects.c','tools/test_wide_remaining_actors.c','tools/test_wide_additional_producer.c','snesrecomp/tests/ppu/ppu_extra_object_test.c','src/lagoon_wide_entities.c','src/lagoon_wide_entities.h','src/lagoon_wide_multipart.c','src/lagoon_wide_spell3.c','src/lagoon_wide_multipart.h','src/lagoon_wide_spell3.h','src/lagoon_scene_renderer.c','src/lagoon_scene_renderer.h','snesrecomp/runner/src/snes/interp816.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c')
 $bindings=@{}
 foreach($path in $paths){$bindings[$path]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
 @{scope='Original equippedspell uploads all15 OBJ forms, separate5C00 ownership; queued player magic kinds1..8 only when every cell is owned by the completed exact original pack. Index3 BG spell excluded. Original9143/NMI/RTI/modeledDMA and BB4B/C41B create independent metadata; synthetic producer tests use one fresh original room14 loader. Padding bytes are not type variants. Retained resource mutation revokes submitted pixels immediately. Live original cast, sourcepacket and selected-output pixels are separately qualified.';rom_sha256=(Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant();sources=$bindings;runs=$runs} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutDir 'public-tests.json') -Encoding utf8
} finally {Pop-Location}
