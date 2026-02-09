@echo off
REM Complete rebuild and deployment script
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "LOCAL_ENV=%SCRIPT_DIR%\config\local.env.bat"
if exist "%LOCAL_ENV%" call "%LOCAL_ENV%"

if "%CMAKE_EXE%"=="" set "CMAKE_EXE=cmake"
if "%NINJA_EXE%"=="" set "NINJA_EXE=ninja"

if "%MO2_PLUGINS_DIR%"=="" (
  echo ERROR: MO2_PLUGINS_DIR is not set. Use config\local.env.bat or an environment variable.
  exit /b 1
)

if "%MO2_ROOT%"=="" (
  set "MO2_ROOT=%MO2_PLUGINS_DIR%\.."
)

echo ========================================
echo MO2 Game Plugins - Rebuild and Test
echo ========================================

REM Set up MSVC environment if configured
if not "%VCVARS_BAT%"=="" (
  call "%VCVARS_BAT%"
  if %ERRORLEVEL% neq 0 (
    echo ERROR: Could not initialize MSVC environment
    exit /b 1
  )
)

cd /d "%SCRIPT_DIR%"
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
set "CMAKE_ARGS=-B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
if not "%QT_ROOT%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_PREFIX_PATH=%QT_ROOT%"
if not "%VCPKG_ROOT%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if not "%MO2_UIBASE_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_PATH=%MO2_UIBASE_PATH%"
if not "%MO2_UIBASE_LIB%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_LIB=%MO2_UIBASE_LIB%"
if not "%MO2_SRC_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_SRC_PATH=%MO2_SRC_PATH%"
"%CMAKE_EXE%" %CMAKE_ARGS%
if %ERRORLEVEL% neq 0 (
  echo ERROR: CMake configuration failed
  exit /b 1
)

echo.
echo ========================================
echo Building with Ninja...
echo ========================================
cd build
"%NINJA_EXE%" -j4
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
    copy /Y build\bin\Release\plugins\*.dll "%MO2_PLUGINS_DIR%\"
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

start "" "%MO2_ROOT%\ModOrganizer.exe"
echo MO2 launched in background
echo.
echo Done.
