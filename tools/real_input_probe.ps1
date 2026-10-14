<#
.SYNOPSIS
Drives the port with REAL key events (SendInput/keybd_event) and reports
whether the guest noticed.

.DESCRIPTION
An input script injects pad words below the event layer: TickScript() ORs a
word into the frame's input. That proves the guest reads the pad, and proves
nothing about the path a player takes -- SDL device open, [KeyMap]/keybinds.ini,
the [Controller] seat a pad is assigned to, the guest word composition. Those
are exactly the layers a person pressing keys on a keyboard hits first.

This starts the game in a real window, waits for the scene to be interactive,
pushes real keyboard events into the foreground window, and diffs the
per-present checksum stream against a run of the same length with nobody
touching the keyboard. Identical streams mean the player's keystrokes never
reached the guest, whatever the config says.

Only keybd_event is used (no struct marshalling): a press is a down event and
an up event 60 ms apart, which is what a human tapping a key produces.

.EXAMPLE
.\tools\real_input_probe.ps1                       # keys
.\tools\real_input_probe.ps1 -NoKeys               # control
# then: compare build\shots-keys\presents.csv with build\shots-keys-none\presents.csv
#>
[CmdletBinding()]
param(
    [int]$Frames = 900,
    [int]$WarmupSeconds = 4,
    [int]$Presses = 14,
    [string]$Tag = 'keys',
    [string]$Exe = 'build\LagoonSNESRecomp.exe',
    [ValidateSet('send', 'post')] [string]$Via = 'send',
    # Real drivers are the point of the tool, but being able to swap one out
    # separates "the window perturbs the guest" from "the audio device does".
    [string]$VideoDriver = 'windows',
    [string]$AudioDriver = 'directsound',
    [switch]$NoKeys
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$build = Join-Path $repo 'build'
$rom = if ($env:SNESRECOMP_ROM) { $env:SNESRECOMP_ROM }
       else { Join-Path (Split-Path -Parent $repo) 'Lagoon (USA).sfc' }
$exe = if ([IO.Path]::IsPathRooted($Exe)) { $Exe } else { Join-Path $repo $Exe }

if (-not ('K32' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class K32 {
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
  // Windows refuses SetForegroundWindow from a background process (foreground
  // lock). Sharing input queue ownership with the current foreground window's
  // thread is the documented way in, and without it keybd_event lands on
  // whatever we last saw, not on the game.
  public static bool Focus(IntPtr hWnd) {
    uint fgpid;
    IntPtr fg = GetForegroundWindow();
    uint fgtid = GetWindowThreadProcessId(fg, out fgpid);
    uint curtid = GetCurrentThreadId();
    AttachThreadInput(curtid, fgtid, true);
    bool ok = SetForegroundWindow(hWnd);
    AttachThreadInput(curtid, fgtid, false);
    return ok;
  }
  public static bool IsFocused(IntPtr hWnd) { return GetForegroundWindow() == hWnd; }
  [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr hWnd, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern uint MapVirtualKeyW(uint vk, uint mapType);
  // Focus-independent delivery: WM_KEYDOWN/WM_KEYUP straight to the window, with
  // the lparam a real driver would produce (scan code in bits 16-23, transition
  // bit 30 set on the release). Foreground-lock rules mean keybd_event lands on
  // the terminal from a background agent no matter what SetForegroundWindow
  // returns; a posted message reaches the window procedure either way.
  public static void PostKey(IntPtr hWnd, byte vk, bool down) {
    uint scan = MapVirtualKeyW(vk, 0);
    ulong lp = (ulong)scan << 16 | (down ? 0UL : (1UL << 30)) | 1UL;
    PostMessageW(hWnd, down ? 0x0100u : 0x0101u, (IntPtr)vk, (IntPtr)lp);
  }
  public static void Press(byte vk) {
    keybd_event(vk, 0, 0, UIntPtr.Zero);
    System.Threading.Thread.Sleep(60);
    keybd_event(vk, 0, 2, UIntPtr.Zero);
  }
}
'@
}

# X = SNES A, Return = Start, per the generated keybinds.ini [player1].
# Down is held longer at the end: a title screen that only reacts to a
# direction is still a title screen a player would call broken.
$vk = [ordered]@{ x = 0x58; ret = 0x0D; down = 0x28 }

$dir = Join-Path $build ("shots-{0}" -f $Tag)
Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$save = 'saves\{0}' -f $Tag
Remove-Item -Recurse -Force (Join-Path $build ($save -replace '/', '\')) -ErrorAction SilentlyContinue

# Real drivers: this is the point of the test, so no dummy video or audio.
$env:SDL_VIDEODRIVER = 'windows'
$env:SDL_AUDIODRIVER = 'directsound'
$env:SNESRECOMP_RUN_FRAMES = "$Frames"
$env:SNESRECOMP_PRESENT_LOG = (Join-Path $dir 'presents.csv')
$env:SNESRECOMP_SCREENSHOT = $null
$env:SNESRECOMP_DUMP_DIR = $null

$p = Start-Process -FilePath $exe -WorkingDirectory $build -PassThru `
      -ArgumentList @('--no-launcher', '--rom', ('"{0}"' -f $rom), '--save-root', $save) `
      -RedirectStandardError (Join-Path $dir 'run.log') -RedirectStandardOutput (Join-Path $dir 'run.out')

Start-Sleep -Seconds $WarmupSeconds
if (-not $NoKeys) {
    # Without this the test proves nothing: a press that lands on the terminal
    # looks identical, downstream, to a port that drops every key.
    $hwnd = $p.MainWindowHandle
    [K32]::Focus($hwnd) | Out-Null
    Start-Sleep -Milliseconds 400
    "focus: hwnd=$hwnd focused=$([K32]::IsFocused($hwnd))"
    $seq = @('x','x','ret','x','down','x','ret','down','x','x','ret','down','x','ret')
    for ($i = 0; $i -lt [Math]::Min($Presses, $seq.Count); $i++) {
        if ($Via -eq 'post') {
            [K32]::PostKey($hwnd, $vk[$seq[$i]], $true)
            Start-Sleep -Milliseconds 60
            [K32]::PostKey($hwnd, $vk[$seq[$i]], $false)
        } else {
            [K32]::Press($vk[$seq[$i]])
        }
        Start-Sleep -Milliseconds 220
    }
    "focus at end: $([K32]::IsFocused($hwnd))"
}
if (-not $p.WaitForExit(60000)) { Stop-Process -Id $p.Id -Force }

$csv = Join-Path $dir 'presents.csv'
$n = if (Test-Path $csv) { (Get-Content $csv | Measure-Object -Line).Lines } else { 0 }
"== ${Tag}: $n presents, exit $($p.ExitCode) -> $csv"
if ($NoKeys) { "== (control: nothing was pressed)" }
