param([Parameter(Mandatory=$true)][string]$OutDir,[string]$Rom='',[string]$LoaderDumpPrefix='')
$ErrorActionPreference='Stop'
$OutDir=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
if($Rom){$Rom=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Rom)}
if($LoaderDumpPrefix){$LoaderDumpPrefix=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($LoaderDumpPrefix)}
$repo=Split-Path -Parent $PSScriptRoot
. (Join-Path (Split-Path -Parent $repo) 'env.ps1')
if(-not $Rom){$Rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'}
if((Get-FileHash -LiteralPath $Rom).Hash.ToLowerInvariant() -ne '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'){throw 'Original pinned Lagoon USA ROM required'}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Push-Location $repo
try {
 $exe=Join-Path $OutDir 'multipart-source.exe'
 & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_multipart_source.c src/lagoon_wide_multipart.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $exe
 if($LASTEXITCODE){throw 'Multipart source oracle compilation failed'}
 $sourceOutput=@(& $exe $Rom)
 if($LASTEXITCODE){throw 'Multipart source oracle failed'}
 $sourceOutput | ForEach-Object {Write-Host $_}
 $producerOutput=@()
 if($LoaderDumpPrefix){
  $producer=Join-Path $OutDir 'multipart-producer.exe'
  $flags=@('-std=c11','-O2','-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-Isnesrecomp/runner/src/desktop',('-I'+$env:RETCOMM_TOOLCHAIN_DIR+'/deps/include'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
  & $env:CC @flags tools/test_wide_multipart_producer.c src/lagoon_wide_multipart.c src/lagoon_scene_renderer.c snesrecomp/runner/src/snes/ppu.c snesrecomp/runner/src/snes/ppu_legacy.c snesrecomp/runner/src/snes/interp816.c -o $producer
  if($LASTEXITCODE){throw 'Multipart producer compilation failed'}
  $producerOutput=@(& $producer $Rom $LoaderDumpPrefix)
  if($LASTEXITCODE){throw 'Multipart producer failed'}
  $producerOutput | ForEach-Object {Write-Host $_}
 }
 $hashes=@{}
 foreach($file in @('src/lagoon_wide_multipart.c','src/lagoon_wide_multipart.h','src/lagoon_wide_entities.c','src/lagoon_wide_entities.h','tools/test_wide_multipart_source.c','tools/test_wide_multipart_producer.c','tools/test_wide_multipart.ps1','tools/test_wide_remaining_actors.c','tools/test_wide_projectile_producer.c','tools/test_wide_additional_producer.c','snesrecomp/runner/src/snes/interp816.c')){
  $hashes[$file]=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()
 }
 @{scope='Full original C284 multipart display source/OAM oracle for authored simple boss kinds2..15 only. Actual loader graphics and synthetic policy records remain distinct from live campaign proof. Entire carrier refuses on any unowned resource/unknown part; real native OAM and scene/generation guards remain.';source_output=$sourceOutput;producer_output=$producerOutput;source_hashes=$hashes;loader_dump_prefix=$LoaderDumpPrefix} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutDir 'multipart-tests.json') -Encoding utf8
} finally {Pop-Location}
