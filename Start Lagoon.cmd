@echo off
setlocal
set "LAUNCHER=%~dp0launcher\bin\Release\net8.0-windows\lagoon.Companion.exe"
if not exist "%LAUNCHER%" (
  echo Build Companion first: powershell -File scripts\build_launcher.ps1
  pause
  exit /b 1
)
start "" "%LAUNCHER%"
