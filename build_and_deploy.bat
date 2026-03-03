@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "LOCAL_ENV=%SCRIPT_DIR%\config\local.env.bat"
if exist "%LOCAL_ENV%" call "%LOCAL_ENV%"

if "%CMAKE_EXE%"=="" set "CMAKE_EXE=cmake"
if /I "%CMAKE_EXE%"=="cmake" (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  )
)
if "%NINJA_EXE%"=="" set "NINJA_EXE=ninja"
if /I "%NINJA_EXE%"=="ninja" (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
    set "NINJA_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
  ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
    set "NINJA_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
  )
)

if "%VCPKG_ROOT%"=="" if exist "C:\vcpkg" set "VCPKG_ROOT=C:\vcpkg"
if "%VCPKG_ROOT%"=="" (
	set "VCPKG_TOOLCHAIN="
) else (
	set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

if "%VCVARS_BAT%"=="" (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    set "VCVARS_BAT=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
  ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" (
    set "VCVARS_BAT=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
  )
)

cd /d "%SCRIPT_DIR%"
if errorlevel 1 goto error_cd

call :stop_mo2

if not "%VCVARS_BAT%"=="" (
	call "%VCVARS_BAT%" -arch=x64
	if errorlevel 1 goto error_vcvars
)

if exist build rmdir /s /q build
mkdir build
cd build
if errorlevel 1 goto error_build_cd

echo Configuring CMake...
set "CMAKE_ARGS=-G Ninja -DCMAKE_BUILD_TYPE=Release"

if not "%VCPKG_TOOLCHAIN%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN%"
if not "%MO2_UIBASE_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_PATH=%MO2_UIBASE_PATH%"
if not "%MO2_UIBASE_LIB%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_LIB=%MO2_UIBASE_LIB%"
if not "%MO2_SRC_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_SRC_PATH=%MO2_SRC_PATH%"
if not "%QT_ROOT%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DQT_ROOT=%QT_ROOT%"

"%CMAKE_EXE%" .. %CMAKE_ARGS%
if errorlevel 1 goto error_cmake

echo.
echo Building with Ninja...
"%CMAKE_EXE%" --build . --config Release --parallel
if errorlevel 1 goto error_ninja

echo.
echo Build completed successfully!
echo.
echo ========================================
echo Deploying to MO2 plugins folder...
echo ========================================

call :stop_mo2

if "%MO2_PLUGINS_DIR%"=="" (
  echo ERROR: MO2_PLUGINS_DIR is not set. Use config\local.env.bat or an environment variable.
  goto fail
)

if not exist "%MO2_PLUGINS_DIR%" (
	echo ERROR: MO2 plugins folder not found: %MO2_PLUGINS_DIR%
	goto fail
)

if not exist "bin\Release\plugins" (
	echo ERROR: Build output folder not found: bin\Release\plugins
	goto fail
)

copy /Y "bin\Release\plugins\game_*.dll" "%MO2_PLUGINS_DIR%\"
if %ERRORLEVEL% neq 0 (
	echo ERROR: Failed to copy DLLs to %MO2_PLUGINS_DIR%
	goto fail
)

if exist "bin\Release\plugins\xdelta.exe" (
  copy /Y "bin\Release\plugins\xdelta.exe" "%MO2_PLUGINS_DIR%\"
  if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to copy xdelta.exe to %MO2_PLUGINS_DIR%
    goto fail
  )
)

echo ✓ Deployment completed
pause
exit /b 0

:error_cd
echo ERROR: Failed to change to project directory
goto fail

:error_vcvars
echo ERROR: Failed to initialize Visual Studio environment
goto fail

:error_build_cd
echo ERROR: Failed to change to build directory
goto fail

:error_cmake
echo ERROR: CMake configuration failed
goto fail

:error_ninja
echo ERROR: Ninja build failed
goto fail

:fail
echo.
echo Build/deploy failed. Leaving this console open for investigation.
echo You can inspect files and rerun commands from:
echo   %SCRIPT_DIR%
cd /d "%SCRIPT_DIR%"
cmd /k

endlocal

:stop_mo2
echo Closing MO2 if running...
taskkill /F /T /IM ModOrganizer.exe >nul 2>&1
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Get-Process -Name 'ModOrganizer' -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue" >nul 2>&1

for /L %%I in (1,1,20) do (
  tasklist /FI "IMAGENAME eq ModOrganizer.exe" 2>nul | find /I "ModOrganizer.exe" >nul
  if errorlevel 1 goto :stop_mo2_done
  timeout /T 1 /NOBREAK >nul
)

echo ERROR: ModOrganizer.exe is still running and may lock plugin DLLs.
echo Please close MO2 manually, then rerun this script.
goto fail

:stop_mo2_done
exit /b 0
