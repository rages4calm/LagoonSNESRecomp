param([Parameter(Mandatory=$true)][string]$OutDir,[string]$Rom='',[string]$Compiler='',[string]$LoaderDumpPrefix='')
$ErrorActionPreference='Stop'
$OutDir=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
if($Rom){$Rom=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Rom)}
if($LoaderDumpPrefix){$LoaderDumpPrefix=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($LoaderDumpPrefix)}
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
 $environment=Join-Path (Split-Path -Parent $repo) 'env.ps1'
 if(Test-Path -LiteralPath $environment){. $environment}
 if(-not $Compiler){$Compiler=if($env:CC){$env:CC}else{'clang'}}
 if(-not $Rom){$Rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'}
 $identity=@(Select-String -LiteralPath 'rom_identity.txt' -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})"?\s*(?:#.*)?$')
 if($identity.Count -ne 1 -or (Get-FileHash -LiteralPath $Rom).Hash.ToLowerInvariant() -ne $identity[0].Matches[0].Groups[1].Value.ToLowerInvariant()){throw 'Tests require the original ROM pinned in rom_identity.txt'}
 New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
 $exe=Join-Path $OutDir 'spell3-source-ppu.exe'
 $sources=@('tools/test_wide_spell3.c','snesrecomp/runner/src/snes/interp816.c','snesrecomp/runner/src/snes/ppu.c','snesrecomp/runner/src/snes/ppu_legacy.c')
 $flags=@('-std=c11','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-Isrc','-Isnesrecomp/runner/src')
 & $Compiler @flags @sources -o $exe
 if($LASTEXITCODE){throw 'spell3 source/PPU compilation failed'}
 $arguments=@($Rom)
 if($LoaderDumpPrefix){$arguments+=$LoaderDumpPrefix}
 $stdout=@(& $exe @arguments)
 if($LASTEXITCODE){throw 'spell3 source/PPU oracle failed'}
 $stdout | ForEach-Object {Write-Host $_}
 $bindings=@{}
 foreach($file in ($sources+@('tools/test_wide_spell3.ps1','src/lagoon_wide_spell3.c','src/lagoon_wide_spell3.h','src/lagoon_wide_entities.c','src/lagoon_widescreen.c','snesrecomp/runner/src/snes/ppu.h'))){$bindings[$file]=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
 if($LoaderDumpPrefix){$bindings[$LoaderDumpPrefix+'.vram.bin']=(Get-FileHash -LiteralPath ($LoaderDumpPrefix+'.vram.bin')).Hash.ToLowerInvariant()}
 @{scope='Original equipped slot3 BG resource and finite fullscreen image. Actual original loader/decompressor, native NMI/RTI, three ordered original map transfers and original black/white palette paths. Both native PPU renderers provide independent backdrop witnesses across all16 brightness levels. Separate synthetic BG3 glyph/palette/flip/priority variations prove all2bpp colors map to uniform pulse backdrop; optional fresh original-loader VRAM adds actual HUD map/font checks. Native256 artwork remains untouched. Tests do not certify uninterrupted cast/runtime/campaign support.';rom_sha256=(Get-FileHash -LiteralPath $Rom).Hash.ToLowerInvariant();sources=$bindings;stdout=$stdout;executable_sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant();arguments=$arguments;flags=$flags} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutDir 'spell3-tests.json') -Encoding utf8
} finally {Pop-Location}
