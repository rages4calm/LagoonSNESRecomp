internal static class PackageUpdateHelper
{
    // Runs outside the installation so Windows can release the launcher's DLLs.
    // Windows PowerShell 5.1 only; no installed .NET SDK/Python is required.
    internal const string Script = """
param([Parameter(Mandatory=$true)][string]$PlanPath,
      [Parameter(Mandatory=$true)][string]$PlanHash,
      [int]$WaitProcessId=0,[switch]$Headless,[int]$StopAfter=0)
$ErrorActionPreference='Stop'
$env:PSModulePath=Join-Path $PSHOME 'Modules'
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -Force
$cmp=[StringComparison]::OrdinalIgnoreCase
function Hash([string]$path) {
    $stream=[IO.File]::OpenRead($path)
    try { $algorithm=[Security.Cryptography.SHA256]::Create(); try { return ([BitConverter]::ToString($algorithm.ComputeHash($stream))).Replace('-','').ToLowerInvariant() } finally { $algorithm.Dispose() } }
    finally { $stream.Dispose() }
}
function NoLinks([string]$path) {
    $cursor=[IO.Path]::GetFullPath($path)
    while($cursor) {
        if(([IO.File]::Exists($cursor) -or [IO.Directory]::Exists($cursor)) -and (([IO.File]::GetAttributes($cursor) -band [IO.FileAttributes]::ReparsePoint) -ne 0)) { throw "Linked update path refused: $cursor" }
        $cursor=[IO.Path]::GetDirectoryName($cursor)
    }
}
function Relative([string]$name) {
    $name=$name.Replace('\','/')
    if($name.Length -gt 240 -or $name.StartsWith('/') -or $name.Contains(':') -or $name.Contains([char]0)) { throw 'Unsafe update path.' }
    $parts=$name.Split('/')
    foreach($part in $parts) {
        if(!$part -or $part -eq '.' -or $part -eq '..' -or $part -ne $part.TrimEnd(' ','.') -or $part.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or $part -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)') { throw 'Unsafe update path.' }
    }
    if($parts[0] -in @('local','local-x68000','Inputs','roms','saves','states','validation','build','.git','.lagoon-updater','iplrom.dat','cgrom.dat') -or [IO.Path]::GetExtension($name) -in @('.sfc','.smc','.dim','.xdf','.rom','.rrstate','.srm','.sav')) { throw 'User data cannot be updated.' }
    return $name
}
function Resolve([string]$root,[string]$name) {
    $root=[IO.Path]::GetFullPath($root).TrimEnd('\')+'\'
    $full=[IO.Path]::GetFullPath([IO.Path]::Combine($root,(Relative $name).Replace('/','\')))
    if(!$full.StartsWith($root,$cmp)) { throw 'Update path escaped destination.' }
    NoLinks $full
    return $full
}
function AtomicText([string]$path,[string]$text) {
    NoLinks $path
    $temp=$path+'.new-'+[Guid]::NewGuid().ToString('N')
    [IO.File]::WriteAllText($temp,$text,(New-Object Text.UTF8Encoding($false)))
    if([IO.File]::Exists($path)) { [IO.File].GetMethod('Replace',[type[]]@([string],[string],[string])).Invoke($null,[object[]]@($temp,$path,$null)) } else { [IO.File]::Move($temp,$path) }
}
function Json([string]$path,$value) { AtomicText $path ($value | ConvertTo-Json -Depth 12) }
function CopyAtomic([string]$source,[string]$target,[string]$expected) {
    NoLinks $source; NoLinks $target
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
    NoLinks $target
    # Same-volume temporary file is required for atomic File.Replace.
    $temp=$target+'.lagoon-update-'+[Guid]::NewGuid().ToString('N')+'.tmp'
    [IO.File]::Copy($source,$temp,$false)
    if((Hash $temp) -ne $expected) { throw 'Staged copy did not match its verified hash.' }
    if([IO.File]::Exists($target)) { [IO.File].GetMethod('Replace',[type[]]@([string],[string],[string])).Invoke($null,[object[]]@($temp,$target,$null)) } else { [IO.File]::Move($temp,$target) }
}
$result=$null; $stateDir=$null; $root=$null; $plan=$null; $gate=$null
try {
    NoLinks $PlanPath
    if((Hash $PlanPath) -ne $PlanHash) { throw 'The update plan changed. Download the update again.' }
    $plan=[IO.File]::ReadAllText($PlanPath) | ConvertFrom-Json
    if($plan.Format -ne 1 -or $plan.Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Unsupported update plan.' }
    $root=[IO.Path]::GetFullPath($plan.Root); $stage=[IO.Path]::GetFullPath($plan.Stage)
    NoLinks $root; NoLinks $stage
    if(![IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($PlanPath)).Equals($stage,$cmp) -or $root.Equals($stage,$cmp) -or $stage.StartsWith($root.TrimEnd('\')+'\',$cmp) -or $root.StartsWith($stage.TrimEnd('\')+'\',$cmp)) { throw 'Invalid staging directory.' }
    for($directory=New-Object IO.DirectoryInfo($root);$null -ne $directory;$directory=$directory.Parent) {
        if([IO.Directory]::Exists([IO.Path]::Combine($directory.FullName,'.git')) -or [IO.File]::Exists([IO.Path]::Combine($directory.FullName,'.git'))) { throw 'Source checkouts cannot be updated in place.' }
    }
    $result=[IO.Path]::Combine($stage,'result.json')
    if($WaitProcessId -gt 0) {
        try { $parent=[Diagnostics.Process]::GetProcessById($WaitProcessId); if(!$parent.WaitForExit(600000)) { throw 'Close Lagoon Companion, then resume the update.' } }
        catch [ArgumentException] { }
    }
    $manifest=[IO.Path]::Combine($root,'windows-preview.json')
    NoLinks $manifest
    $manifestHash=Hash $manifest
    if($manifestHash -eq $plan.AfterManifest) { Json $result @{passed=$true;already_complete=$true}; exit 0 }
    if($manifestHash -ne $plan.BeforeManifest) { throw 'The package manifest changed. No files were overwritten.' }
    $payload=[IO.Path]::Combine($stage,'payload');NoLinks $payload
    $incoming=[IO.Path]::Combine($payload,'windows-preview.json')
    if((Hash $incoming) -ne $plan.AfterManifest) { throw 'The staged manifest changed.' }
    $old=[IO.File]::ReadAllText($manifest) | ConvertFrom-Json
    $new=[IO.File]::ReadAllText($incoming) | ConvertFrom-Json
    if($new.version -ne $plan.Version -or $new.package -ne ('Lagoon-Companion-'+$plan.Version+'-win-x64') -or $new.mode -ne 'prebuilt-own-rom') { throw 'Unexpected update package.' }
    $newFiles=@{};foreach($row in $new.files.PSObject.Properties) { $relative=Relative $row.Name; if($newFiles.ContainsKey($relative)) { throw 'Duplicate package path.' };$newFiles[$relative]=[string]$row.Value }
    # Resume accepts only exact original or exact incoming bytes, never edits.
    foreach($row in $old.files.PSObject.Properties) {
        $target=Resolve $root $row.Name
        if([IO.File]::Exists($target)) {
            $actual=Hash $target
            if($actual -ne $row.Value -and (!$newFiles.ContainsKey($row.Name) -or $actual -ne $newFiles[$row.Name])) { throw ('Managed file changed locally: '+$row.Name) }
        }
    }
    $seen=@{}; $parents=@{}
    foreach($file in $plan.Files) {
        $relative=Relative $file.Relative
        if($seen.ContainsKey($relative) -or !$newFiles.ContainsKey($relative) -or $newFiles[$relative] -ne $file.After) { throw 'Invalid update file plan.' };$seen[$relative]=$true
        $source=Resolve $payload $relative;$target=Resolve $root $relative
        if((Hash $source) -ne $file.After) { throw 'A staged payload file changed.' }
        if([IO.Directory]::Exists($target)) { throw ('A user folder blocks an update file: '+$relative) }
        if([IO.File]::Exists($target)) {
            $actual=Hash $target
            if($actual -ne $file.After -and (!$file.Before -or $actual -ne $file.Before)) { throw ('An untracked or edited file would be overwritten: '+$relative) }
            if(([IO.File]::GetAttributes($target) -band [IO.FileAttributes]::ReadOnly) -ne 0) { throw ('Read-only update file: '+$relative) }
            $lock=[IO.File]::Open($target,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$lock.Dispose()
        }
        $parent=[IO.Path]::GetDirectoryName($target)
        while(![IO.Directory]::Exists($parent)) { $parent=[IO.Path]::GetDirectoryName($parent) }
        $parents[$parent]=$true
    }
    # Neither game is terminated. Locked executables stop the update preflight.
    foreach($relative in @('game/LagoonSNESRecomp.exe','local-x68000/retrorecomp-expedition/Lagoon - Atlas.exe','local-x68000/retrorecomp-expedition/Lagoon - English.exe','lagoon.Companion.exe')) {
        $path=[IO.Path]::Combine($root,$relative.Replace('/','\'));NoLinks $path
        if([IO.File]::Exists($path)) {
            foreach($process in [Diagnostics.Process]::GetProcessesByName([IO.Path]::GetFileNameWithoutExtension($path))) {
                try {
                    if(!$process.HasExited -and [IO.Path]::GetFullPath($process.MainModule.FileName).Equals([IO.Path]::GetFullPath($path),$cmp)) { throw 'Close all Lagoon games and launchers before updating. The updater will not terminate them.' }
                } finally { $process.Dispose() }
            }
            $lock=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$lock.Dispose()
        }
    }
    foreach($parent in $parents.Keys) {
        $probe=[IO.Path]::Combine($parent,'.lagoon-write-probe-'+[Guid]::NewGuid().ToString('N'))
        $stream=[IO.File]::Open($probe,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None);$stream.Dispose();[IO.File]::Delete($probe)
    }
    $stateDir=[IO.Path]::Combine($root,'.lagoon-updater');NoLinks $stateDir
    [IO.Directory]::CreateDirectory($stateDir) | Out-Null
    $gate=[IO.File]::Open([IO.Path]::Combine($stateDir,'lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    $statePath=[IO.Path]::Combine($stateDir,'state.json');$resume=[IO.Path]::Combine($stateDir,'Resume Lagoon Update.cmd')
    $helper=[IO.Path]::Combine($stage,'apply.ps1')
    $command='@echo off'+"`r`n"+'"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "'+$helper+'" -PlanPath "'+$PlanPath+'" -PlanHash "'+$PlanHash+'"'+"`r`n"+'pause'+"`r`n"
    if([IO.File]::Exists($statePath)) {
        $previous=[IO.File]::ReadAllText($statePath) | ConvertFrom-Json
        if($previous.Root -ne $root -or (!$previous.Complete -and $previous.PlanHash -ne $PlanHash)) { throw 'Another incomplete update must be resumed first.' }
        if([IO.File]::Exists($resume) -and (Hash $resume) -ne $previous.ResumeHash) { throw 'The recovery command was modified. No files were overwritten.' }
    } elseif([IO.File]::Exists($resume)) { throw 'An untracked recovery command already exists.' }
    AtomicText $resume $command
    $state=@{Format=1;Root=$root;Plan=$PlanPath;PlanHash=$PlanHash;Version=$plan.Version;Complete=$false;ResumeHash=(Hash $resume)}
    Json $statePath $state
    $count=0
    foreach($file in $plan.Files) {
        $target=Resolve $root $file.Relative
        if(![IO.File]::Exists($target) -or (Hash $target) -ne $file.After) { CopyAtomic (Resolve $payload $file.Relative) $target $file.After }
        $count++;Json ([IO.Path]::Combine($stage,'journal.json')) @{completed=$count;total=@($plan.Files).Count;manifest_written=$false}
        if($StopAfter -gt 0 -and $count -eq $StopAfter) { throw 'Test interruption after payload file.' }
    }
    # The old manifest remains authoritative until every managed file is ready.
    CopyAtomic $incoming $manifest $plan.AfterManifest
    $state.Complete=$true;Json $statePath $state
    Json $result @{passed=$true;version=$plan.Version;files=$count;manifest_written=$true}
    # Release full archive/payload immediately. Keep only small resume receipts.
    try {
    $directories=@{}
    foreach($relative in (@($newFiles.Keys)+@('windows-preview.json'))) {
        $path=Resolve $payload $relative
        if([IO.File]::Exists($path)) { [IO.File]::Delete($path) }
        $directory=[IO.Path]::GetDirectoryName($path)
        while($directory -and $directory.StartsWith($payload+'\',$cmp)) { $directories[$directory]=$true;$directory=[IO.Path]::GetDirectoryName($directory) }
    }
    foreach($directory in @($directories.Keys | Sort-Object Length -Descending)) { if([IO.Directory]::Exists($directory) -and [IO.Directory]::GetFileSystemEntries($directory).Length -eq 0) { [IO.Directory]::Delete($directory,$false) } }
    $archive=[IO.Path]::Combine($stage,'download.zip');NoLinks $archive;if([IO.File]::Exists($archive)) { [IO.File]::Delete($archive) }
    } catch { Write-Warning ('Update installed; temporary-download cleanup could not finish: '+$_.Exception.Message) }
    if(!$Headless) { $start=New-Object Diagnostics.ProcessStartInfo; $start.FileName=[IO.Path]::Combine($root,'lagoon.Companion.exe');$start.WorkingDirectory=$root;$start.UseShellExecute=$false;[Diagnostics.Process]::Start($start) | Out-Null }
    Write-Output 'Lagoon update complete. Saves and settings were preserved.'
} catch {
    $message=$_.Exception.Message
    if($result) { try { Json $result @{passed=$false;error=$message;resumable=($null -ne $stateDir)} } catch {} }
    Write-Error $message -ErrorAction Continue
    if(!$Headless) {
        $next='Close Lagoon and open Check for updates to try again.'
        if($stateDir -and [IO.File]::Exists([IO.Path]::Combine($stateDir,'Resume Lagoon Update.cmd'))) { $next='Use the recovery command in the release folder: .lagoon-updater\Resume Lagoon Update.cmd.' }
        Add-Type -AssemblyName System.Windows.Forms
        [Windows.Forms.MessageBox]::Show(('The update paused: '+$message+"`n`n"+$next+"`nYour saves and settings were not replaced."),'Lagoon update') | Out-Null
    }
    exit 1
} finally { if($gate) { $gate.Dispose() } }
""";
}
