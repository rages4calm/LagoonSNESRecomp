@echo off
setlocal
set "launcher=%~dp0local-x68000\companion\lagoon.Companion.exe"
if not exist "%~dp0local-x68000\retrorecomp-expedition\lagoon-atlas-install.json" goto setup
if not exist "%launcher%" goto setup
set "COMPANION_DATA_ROOT=%~dp0local-x68000\companion-data"
start "" "%launcher%" --game x68000
exit /b 0
:setup
echo Extract the complete Windows preview ZIP and run Setup X68000.cmd first.
pause
exit /b 1
