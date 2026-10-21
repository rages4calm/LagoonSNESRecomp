param([Parameter(Mandatory=$true)][string]$OutDir,
 [Parameter(Mandatory=$true)][string]$Room40DumpPrefix,[string]$Rom='')
$ErrorActionPreference='Stop'
$OutDir=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
$Room40DumpPrefix=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Room40DumpPrefix)
if($Rom){$Rom=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Rom)}
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
 . ../env.ps1
 if(-not $Rom){$Rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'}
 $identity=Select-String -LiteralPath rom_identity.txt -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})'
 if($identity.Count -ne 1 -or (Get-FileHash -LiteralPath $Rom).Hash.ToLowerInvariant() -ne $identity.Matches[0].Groups[1].Value.ToLowerInvariant()){throw 'Pinned original ROM required'}
 New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
 $source=Join-Path $OutDir 'prop-shake-source.exe'
 & $env:CC -std=c11 -O2 -DLAGOON_WIDE_ENTITIES_DECODER_ONLY -Isrc -Isnesrecomp/runner/src tools/test_wide_prop_shake_source.c src/lagoon_wide_entities.c src/lagoon_wide_multipart.c snesrecomp/runner/src/snes/interp816.c -o $source
 if($LASTEXITCODE){throw 'Prop shake source compilation failed'}
 $sourceOutput=@(& $source $Rom ($Room40DumpPrefix+'.wram.bin'))
 if($LASTEXITCODE){throw 'Prop shake source failed'}
 $sourceOutput | ForEach-Object {Write-Host $_}
 $flags=@('-std=c11','-O2','-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-Isnesrecomp/runner/src/desktop',('-I'+$env:RETCOMM_TOOLCHAIN_DIR+'/deps/include'),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
 $producer=Join-Path $OutDir 'prop-shake-producer.exe'
 & $env:CC @flags tools/test_wide_prop_shake_producer.c src/lagoon_wide_multipart.c src/lagoon_wide_spell3.c src/lagoon_scene_renderer.c snesrecomp/runner/src/snes/ppu.c snesrecomp/runner/src/snes/ppu_legacy.c snesrecomp/runner/src/snes/interp816.c -o $producer
 if($LASTEXITCODE){throw 'Prop shake producer compilation failed'}
 $producerOutput=@(& $producer $Rom $Room40DumpPrefix)
 if($LASTEXITCODE){throw 'Prop shake producer failed'}
 $producerOutput | ForEach-Object {Write-Host $_}
 $hashes=@{}
 foreach($file in @('src/lagoon_wide_multipart.c','src/lagoon_wide_multipart.h','src/lagoon_wide_entities.c','src/lagoon_wide_entities.h','src/lagoon_wide_spell3.c','src/lagoon_wide_spell3.h','src/lagoon_scene_renderer.c','src/lagoon_scene_renderer.h','tools/test_wide_prop_shake_source.c','tools/test_wide_prop_shake_producer.c','tools/test_wide_prop_shake.ps1','tools/test_wide_multipart_composite_producer.c','tools/test_wide_multipart_producer.c','tools/test_wide_projectile_producer.c','tools/test_wide_additional_producer.c','tools/test_wide_remaining_actors.c','snesrecomp/runner/src/snes/interp816.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c')){$hashes[$file]=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
 @{scope='Original room40 prop C44E draws before D8DA/DDAD advances its finite shake table. Queued authored prop only, postcounter2..42 and exact current-offset/source/global/slot/pose proof. NativeOAM/ownedresource/camera/generation guards remain; public/current/far decoding unchanged. Actual loader graphics, synthetic counter/camera policy cases; separate live quiet route required.';source_output=$sourceOutput;producer_output=$producerOutput;source_hashes=$hashes;room40_prefix=$Room40DumpPrefix} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $OutDir 'prop-shake-tests.json') -Encoding utf8
} finally {Pop-Location}
