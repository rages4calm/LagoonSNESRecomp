param(
 [Parameter(Mandatory=$true)][string]$OutDir,
 [Parameter(Mandatory=$true)][string]$ActorSnapshots
)
$ErrorActionPreference='Stop'
$caller=(Get-Location).Path
$OutDir=[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($OutDir)){$OutDir}else{Join-Path $caller $OutDir}))
$ActorSnapshots=[IO.Path]::GetFullPath($(if([IO.Path]::IsPathRooted($ActorSnapshots)){$ActorSnapshots}else{Join-Path $caller $ActorSnapshots}))
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
 . ../env.ps1
 $rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'
 $identity=@(Select-String -LiteralPath 'rom_identity.txt' -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})"?\s*(?:#.*)?$')
 if($identity.Count -ne 1 -or (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant() -ne $identity[0].Matches[0].Groups[1].Value.ToLowerInvariant()){throw 'Tests require the original ROM pinned in rom_identity.txt'}
 New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
 $runs=@()
 function Invoke-Checked([string]$Name,[string[]]$Sources,[string[]]$Flags,[string[]]$Arguments){
  $exe=Join-Path $OutDir ($Name+'.exe')
  & $env:CC -std=c11 -O2 @Flags @Sources -o $exe
  if($LASTEXITCODE){throw ($Name+' compile failed')}
  $stdout=@(& $exe @Arguments)
  if($LASTEXITCODE){throw ($Name+' failed')}
  $stdout | ForEach-Object {Write-Host $_}
  $script:runs+=@{name=$Name;stdout=$stdout;executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant();sources=$Sources;flags=$Flags;arguments=$Arguments}
 }
 $pure=@('-DLAGOON_WIDE_ENTITIES_DECODER_ONLY','-Isrc','-Isnesrecomp/runner/src')
 Invoke-Checked 'current-source' @('tools/test_wide_current_actor_source.c','src/lagoon_wide_entities.c','snesrecomp/runner/src/snes/interp816.c') $pure @($rom)
 $flags=@('-DSDL_MAIN_HANDLED=1','-DSNESRECOMP_SDL3=1','-DSNESRECOMP_REVERSE_DEBUG=0','-Isrc','-Isnesrecomp/runner/src','-Isnesrecomp/runner/src/snes','-Isnesrecomp/runner/src/desktop',"-I$env:RETCOMM_TOOLCHAIN_DIR/deps/include",'-ffunction-sections','-fdata-sections','-Wl,--gc-sections')
 Invoke-Checked 'current-producer' @('tools/test_wide_current_actor_producer.c','src/lagoon_wide_multipart.c','src/lagoon_scene_renderer.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c') $flags @($rom,$ActorSnapshots)
 $paths=@('tools/test_wide_current_actors.ps1','tools/test_wide_current_actor_source.c','tools/test_wide_current_actor_producer.c','tools/test_wide_remaining_actors.c','tools/test_wide_additional_producer.c','snesrecomp/tests/ppu/ppu_extra_object_test.c','src/lagoon_wide_entities.c','src/lagoon_wide_entities.h','src/lagoon_wide_multipart.c','src/lagoon_wide_multipart.h','src/lagoon_scene_renderer.c','src/lagoon_scene_renderer.h','snesrecomp/runner/src/snes/interp816.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c')
 $bindings=@{}
 foreach($path in $paths){$bindings[$path]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
 @{scope='Every admitted finite current frame: original generic/OAM and kind-specific draw metadata, real projection/culling; synthetic unseen/changed producer and pixel/refusal policy. Actual spawned off/auto routes remain separate.';rom_sha256=(Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash.ToLowerInvariant();sources=$bindings;runs=$runs} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutDir 'public-tests.json') -Encoding utf8
} finally {Pop-Location}
