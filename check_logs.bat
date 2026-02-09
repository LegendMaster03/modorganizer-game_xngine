@echo off
setlocal enabledelayedexpansion

set logfile=%LOCALAPPDATA%\ModOrganizer\ModOrganizer.log

echo Checking MO2 process...
tasklist | findstr /I ModOrganizer >nul
if %ERRORLEVEL% equ 0 (
  echo ✓ MO2 is running
) else (
  echo ✗ MO2 is NOT running - it may have crashed
)

echo.
echo Checking log file at: %logfile%
if exist "%logfile%" (
  echo.
  echo === Last 100 lines of MO2 log ===
  echo.
  powershell -Command "Get-Content '%logfile%' -Tail 100"
  echo.
  echo === Searching for [GameRedguard] messages ===
  powershell -Command "Select-String '\[GameRedguard\]' '%logfile%' -Context 2 | Select-Object -Last 20"
) else (
  echo ✗ Log file not found!
  echo.
  echo Checking for logs in subdirectories...
  dir /s "%LOCALAPPDATA%\ModOrganizer\*.log" 2>nul | findstr log
)

echo.
pause
