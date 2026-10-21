param([Parameter(Mandatory=$true)][string]$Rom)
# Prepare the ROM-free prebuilt preview. No downloads, compiler or global installs.
$ErrorActionPreference = 'Stop'
$env:PSModulePath = Join-Path $PSHOME 'Modules'
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -Force
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$ExpectedRom = '46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb'
$Rom = (Resolve-Path -LiteralPath $Rom).Path
if ((Get-Item -LiteralPath $Rom).Length -ne 1048576 -or (Get-FileHash -LiteralPath $Rom -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ExpectedRom) {
    throw 'Select the clean, unheadered 1 MiB Lagoon USA ROM. Patched or different revisions are unsupported.'
}
$ContractPath = Join-Path $ProjectRoot 'windows-preview.json'
if (!(Test-Path -LiteralPath $ContractPath)) { throw 'Run this from the complete extracted Windows preview package.' }
$Contract = Get-Content -LiteralPath $ContractPath -Raw | ConvertFrom-Json
if ($Contract.mode -ne 'prebuilt-own-rom') { throw 'This setup requires the playable Windows preview package.' }
foreach ($File in $Contract.files.PSObject.Properties) {
    $Path = Join-Path $ProjectRoot $File.Name
    if (!(Test-Path -LiteralPath $Path) -or (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $File.Value) { throw "Package file missing or changed: $($File.Name). Extract the original preview again." }
}
$Local = Join-Path $ProjectRoot 'local'
$ReadyPath = Join-Path $Local 'setup-ready.json'
$Exe = Join-Path $ProjectRoot 'game/LagoonSNESRecomp.exe'
$Config = Join-Path $Local 'config.ini'
if (Test-Path -LiteralPath $ReadyPath) {
    $Ready = Get-Content -LiteralPath $ReadyPath -Raw | ConvertFrom-Json
    if ($Ready.package -ne $Contract.package -or $Ready.game_sha256 -ne $Contract.game_sha256 -or !(Test-Path -LiteralPath $Ready.config)) { throw 'Existing setup belongs to another preview or is incomplete. Extract into a fresh folder; existing saves were preserved.' }
    Write-Output 'Setup already complete. Existing settings and saves were preserved.'; exit 0
}
New-Item -ItemType Directory -Path $Local -Force | Out-Null
# Only tiny, reviewed shader/catalog files are copied. Never copy a ROM, save,
# generated program or executable. Runtime state lives in local/, outside game/.
foreach ($File in $Contract.files.PSObject.Properties) {
    if ($File.Name.StartsWith('game/assets/shaders/') -or $File.Name.StartsWith('game/mods/preloaded/packages/')) {
        $Relative = $File.Name.Substring(5)
        $Destination = Join-Path $Local $Relative
        if (Test-Path -LiteralPath $Destination) {
            if ((Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $File.Value) { throw "Existing local catalog/shader differs: $Relative. Extract into a fresh folder; no local files were overwritten." }
        } else {
            New-Item -ItemType Directory -Path (Split-Path $Destination -Parent) -Force | Out-Null
            Copy-Item -LiteralPath (Join-Path $ProjectRoot $File.Name) -Destination $Destination
        }
    }
}
if (!(Test-Path -LiteralPath $Config)) { Copy-Item -LiteralPath (Join-Path $ProjectRoot 'scripts/preview-default.ini') -Destination $Config }
New-Item -ItemType Directory -Path (Join-Path $Local 'saves') -Force | Out-Null
$Ready = @{package=$Contract.package; executable=$Exe; config=$Config; rom=$Rom; game_sha256=$Contract.game_sha256; generated_manifest_sha256=$Contract.generated_manifest_sha256}
$Temp = Join-Path $Local 'setup-ready.tmp'
$Ready | ConvertTo-Json | Set-Content -LiteralPath $Temp -Encoding UTF8
Move-Item -LiteralPath $Temp -Destination $ReadyPath
Write-Output 'Setup complete. Return to Companion and press Play.'
