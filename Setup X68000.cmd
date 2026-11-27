@echo off
setlocal
if not "%~1"=="" goto command_line
if not exist "%~dp0lagoon.Companion.exe" goto command_line
start "" "%~dp0lagoon.Companion.exe" --game x68000
exit /b 0
:command_line
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0x68000\tools\setup_bootstrap.ps1" %*
set "result=%errorlevel%"
if not "%result%"=="0" echo See docs\BUILDING.md for X68000 setup help.
pause
exit /b %result%
