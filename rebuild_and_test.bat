@echo off
REM Complete rebuild and deployment script
setlocal enabledelayedexpansion

echo ========================================
echo MO2 Game Plugins - Rebuild and Test
echo ========================================

REM Set up MSVC environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if %ERRORLEVEL% neq 0 (
  echo ERROR: Could not initialize MSVC environment
  exit /b 1
)

REM Prepend VS CMake to PATH to override any MSYS2 cmake
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"

cd /d "%~dp0"
echo Current directory: %CD%

REM Clean and rebuild
if exist build (
  echo Cleaning old build...
  rmdir /s /q build
)

echo.
echo ========================================
echo Configuring CMake...
echo ========================================
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_PREFIX_PATH=C:/Qt/6.7.1/msvc2019_64
if %ERRORLEVEL% neq 0 (
  echo ERROR: CMake configuration failed
  exit /b 1
)

echo.
echo ========================================
echo Building with Ninja...
echo ========================================
cd build
ninja -j4
if %ERRORLEVEL% neq 0 (
  echo ERROR: Ninja build failed
  exit /b 1
)
cd ..

echo.
echo ========================================
echo Checking for built DLLs...
echo ========================================
if exist build\bin\Release\plugins (
  dir build\bin\Release\plugins\game_*.dll
  
  echo.
  echo Copying DLLs to MO2 plugins folder...
     copy /Y build\bin\Release\plugins\*.dll C:\Modding\MO2\plugins\
  if %ERRORLEVEL% equ 0 (
    echo ✓ DLLs deployed successfully
  )
) else (
  echo ERROR: Build output directory not found
  exit /b 1
)

echo.
echo ========================================
echo Launching MO2...
echo ========================================
taskkill /IM ModOrganizer.exe /F 2>nul
timeout /t 1 /nobreak >nul 2>&1

echo.
echo To view MO2 logs:
echo   Location: %LOCALAPPDATA%\ModOrganizer\ModOrganizer.log
echo   Search for: [GameRedguard]
echo.

start "" "C:\Modding\MO2\ModOrganizer.exe"
echo MO2 launched in background
echo.
echo Done.
