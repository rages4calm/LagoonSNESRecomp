@echo off
setlocal
where py.exe >nul 2>&1
if errorlevel 1 goto use_python
py -3 -B -P "%~dp0x68000\tools\setup_preview.py" %*
goto finished
:use_python
python -B -P "%~dp0x68000\tools\setup_preview.py" %*
:finished
set "result=%errorlevel%"
if not "%result%"=="0" echo See README.md for prerequisites and input files.
pause
exit /b %result%
