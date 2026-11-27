param([string]$Inputs = '', [string]$Ipl = '', [string]$Cgrom = '')
# Portable, per-user setup runtime. No Python installation or administrator is needed.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$projectRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$runtimeRoot = Join-Path $env:LOCALAPPDATA 'LagoonCompanion\tools\python-3.14.0-amd64'
$archive = Join-Path (Split-Path $runtimeRoot -Parent) 'python-3.14.0-embed-amd64.zip'
$expected = '8d4d3590c10449d78aa4375f534e6d5f3027d67fdc362dd1a882279db6f90fdf'
$url = 'https://www.python.org/ftp/python/3.14.0/python-3.14.0-embed-amd64.zip'
$previousLocation = Get-Location
$runtimeLock = $null
try {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    New-Item -ItemType Directory -Path (Split-Path $runtimeRoot -Parent) -Force | Out-Null
    $runtimeLock = [IO.File]::Open((Join-Path (Split-Path $runtimeRoot -Parent) 'python-setup.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    if (!(Test-Path -LiteralPath $archive) -or (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expected) {
        Write-Output 'Downloading the portable setup runtime (12 MB). Python will not be installed on your computer.'
        $pending = $archive + '.' + [Guid]::NewGuid().ToString('N') + '.download'
        Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $pending -TimeoutSec 120
        if ((Get-FileHash -LiteralPath $pending -Algorithm SHA256).Hash -ne $expected) { throw 'The setup runtime download failed its integrity check. Please try Setup again.' }
        Move-Item -LiteralPath $pending -Destination $archive -Force
    }
    New-Item -ItemType Directory -Path $runtimeRoot -Force | Out-Null
    if ((Get-Item -LiteralPath $runtimeRoot -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'The portable runtime folder must not be a linked directory.' }
    $zip = [IO.Compression.ZipFile]::OpenRead($archive)
    try {
        foreach ($entry in $zip.Entries) {
            if (!$entry.Name) { continue }
            $target = [IO.Path]::GetFullPath((Join-Path $runtimeRoot $entry.FullName))
            if (!$target.StartsWith($runtimeRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe setup runtime archive path.' }
            if (Test-Path -LiteralPath $target) {
                if ((Get-Item -LiteralPath $target -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'A portable runtime file must not be a link.' }
                # The pinned archive is retained so an offline retry can verify every file.
                $source = $entry.Open()
                try { $sha = [Security.Cryptography.SHA256]::Create(); try { $wanted = [BitConverter]::ToString($sha.ComputeHash($source)).Replace('-', '') } finally { $sha.Dispose() } } finally { $source.Dispose() }
                if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -eq $wanted) { continue }
            }
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target, $true)
        }
    } finally { $zip.Dispose() }
    $runtimeLock.Dispose(); $runtimeLock = $null
    if (!$Inputs) { $Inputs = Join-Path $projectRoot 'Inputs\X68000' }
    $pythonArgs = @('-B', '-P', (Join-Path $PSScriptRoot 'setup_preview.py'), '--inputs', $Inputs)
    if ($Ipl) { $pythonArgs += @('--ipl', $Ipl) }
    if ($Cgrom) { $pythonArgs += @('--cgrom', $Cgrom) }
    Write-Output 'Preparing X68000. The remaining portable tools are downloaded automatically the first time.'
    Set-Location -LiteralPath $projectRoot
    & (Join-Path $runtimeRoot 'python.exe') @pythonArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Output 'X68000 is ready. Press Play in Companion.'
} catch {
    Write-Output ('X68000 setup could not finish: ' + $_.Exception.Message)
    exit 1
} finally {
    if ($runtimeLock) { $runtimeLock.Dispose() }
    Set-Location -LiteralPath $previousLocation.Path
}
