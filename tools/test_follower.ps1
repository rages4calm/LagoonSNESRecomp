param([Parameter(Mandatory=$true)][string]$OutDir,[string]$LoaderDump)
$ErrorActionPreference='Stop'
$OutDir=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutDir)
$repo=Split-Path -Parent $PSScriptRoot
Push-Location $repo
try {
  . ../env.ps1
  $rom=Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc'
  $identity=@(Select-String -LiteralPath 'rom_identity.txt' -Pattern '^\s*expected_sha256\s*=\s*"?([0-9a-fA-F]{64})"?\s*(?:#.*)?$')
  if($identity.Count-ne1){throw 'Missing or ambiguous ROM identity'}
  if((Get-FileHash -LiteralPath $rom).Hash.ToLowerInvariant()-ne$identity[0].Matches[0].Groups[1].Value.ToLowerInvariant()){throw 'Original ROM identity mismatch'}
  New-Item -ItemType Directory -Force $OutDir | Out-Null
  $exe=Join-Path $OutDir 'test-follower.exe'
  & $env:CC -std=c11 -O2 -Wall -Wextra -Werror -Isrc -Isnesrecomp/runner/src tools/test_follower.c snesrecomp/runner/src/snes/interp816.c -o $exe
  if($LASTEXITCODE){throw 'Follower source test compile failed'}
  if($LoaderDump){$output=& $exe $rom $LoaderDump}else{$output=& $exe $rom}
  if($LASTEXITCODE){throw 'Follower source test failed'}
  $output
  $files=@('src/lagoon_follower.c','src/lagoon_follower.h','tools/test_follower.c','tools/test_follower.ps1')
  $hashes=@{};foreach($file in $files){$hashes[$file]=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
  $fixture=$null
  if($LoaderDump){$fixture=@{path=$LoaderDump;sha256=(Get-FileHash -LiteralPath $LoaderDump).Hash.ToLowerInvariant();scope='Fresh original-loader room15 actor and map; synthetic original-input continuation is separate from live campaign evidence'}}
  @{schema='Lagoon follower original source v1';result=$output;sources=$hashes;loader_fixture=$fixture;rom_sha256=(Get-FileHash -LiteralPath $rom).Hash.ToLowerInvariant()} | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $OutDir 'source-result.json')
} finally {Pop-Location}
