param([Parameter(Mandatory=$true)][string]$OutDir,[string]$Rom='',
 [string]$Room40DumpPrefix='',[string]$Room41DumpPrefix='')
$ErrorActionPreference='Stop'
$OutDir=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
if($Rom){$Rom=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Rom)}
if($Room40DumpPrefix){$Room40DumpPrefix=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Room40DumpPrefix)}
if($Room41DumpPrefix){$Room41DumpPrefix=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Room41DumpPrefix)}
if([bool]$Room40DumpPrefix -ne [bool]$Room41DumpPrefix){throw 'Supply both original room40/41 dump prefixes for producer tests'}
$repo=Split-Path -Parent $PSScriptRoot
. (Join-Path (Split-Path -Parent $repo) 'env.ps1')
if(-not $Rom){$Rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'}
if((Get-FileHash -LiteralPath $Rom).Hash.ToLowerInvariant() -ne '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'){throw 'Original pinned Lagoon USA ROM required'}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Push-Location $repo
try {
 $exe=Join-Path $OutDir 'composite-source.exe'
 & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_multipart_composite_source.c src/lagoon_wide_multipart.c src/lagoon_wide_entities.c snesrecomp/runner/src/snes/interp816.c -o $exe
 if($LASTEXITCODE){throw 'Composite source oracle compilation failed'}
 $sourceOutput=@(& $exe $Rom)
 if($LASTEXITCODE){throw 'Composite source oracle failed'}
 $sourceOutput | ForEach-Object {Write-Host $_}
 $producerOutput=@()
 if($Room40DumpPrefix){
  $producer=Join-Path $OutDir 'composite-producer.exe'
  $flags=@('-std=c11','-O2','-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-Isnesrecomp/runner/src/desktop',('-I'+$env:RETCOMM_TOOLCHAIN_DIR+'/deps/include'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
  & $env:CC @flags tools/test_wide_multipart_composite_producer.c src/lagoon_wide_multipart.c src/lagoon_wide_spell3.c src/lagoon_scene_renderer.c snesrecomp/runner/src/snes/ppu.c snesrecomp/runner/src/snes/ppu_legacy.c snesrecomp/runner/src/snes/interp816.c -o $producer
  if($LASTEXITCODE){throw 'Composite producer compilation failed'}
  $producerOutput=@(& $producer $Rom $Room40DumpPrefix $Room41DumpPrefix)
  if($LASTEXITCODE){throw 'Composite producer failed'}
  $producerOutput | ForEach-Object {Write-Host $_}
 }
 $hashes=@{}
 foreach($file in @('src/lagoon_wide_multipart.c','src/lagoon_wide_multipart.h','src/lagoon_wide_spell3.c','src/lagoon_wide_spell3.h','src/lagoon_wide_entities.c','src/lagoon_wide_entities.h','tools/test_wide_multipart_composite_source.c','tools/test_wide_multipart_composite_producer.c','tools/test_wide_multipart_composite.ps1','tools/test_wide_multipart_source.c','tools/test_wide_multipart_producer.c','tools/test_wide_remaining_actors.c','tools/test_wide_projectile_producer.c','tools/test_wide_additional_producer.c','snesrecomp/runner/src/snes/interp816.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c')){
  $hashes[$file]=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()
 }
 @{scope='Original full primary boss0/1 display carrier, including all optional source layers. Exact original-opcode/OAM and independent pixel proofs. Actual original loader graphics; arranged policy poses/order remain distinct from live boss coverage. Ordinary decoder64, multipart128, global512 whole-packet buffer unchanged. Unknown scripted11..13/unowned resources remain refused.';source_output=$sourceOutput;producer_output=$producerOutput;source_hashes=$hashes;room40_prefix=$Room40DumpPrefix;room41_prefix=$Room41DumpPrefix} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutDir 'composite-tests.json') -Encoding utf8
} finally {Pop-Location}
