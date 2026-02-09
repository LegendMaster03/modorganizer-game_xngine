@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "LOCAL_ENV=%SCRIPT_DIR%\config\local.env.bat"
if exist "%LOCAL_ENV%" call "%LOCAL_ENV%"

if "%CMAKE_EXE%"=="" set "CMAKE_EXE=cmake"
if "%NINJA_EXE%"=="" set "NINJA_EXE=ninja"

if "%VCPKG_ROOT%"=="" (
	set "VCPKG_TOOLCHAIN="
) else (
	set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

if "%VCVARS_BAT%"=="" (
	set "VCVARS_BAT="
)

cd /d "%SCRIPT_DIR%"
if errorlevel 1 goto error_cd

if not "%VCVARS_BAT%"=="" (
	call "%VCVARS_BAT%"
	if errorlevel 1 goto error_vcvars
)

if exist build rmdir /s /q build
mkdir build
cd build
if errorlevel 1 goto error_build_cd

echo Configuring CMake...
set "CMAKE_ARGS=-G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=%NINJA_EXE%"

if not "%VCPKG_TOOLCHAIN%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN%"
if not "%MO2_UIBASE_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_PATH=%MO2_UIBASE_PATH%"
if not "%MO2_UIBASE_LIB%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_LIB=%MO2_UIBASE_LIB%"
if not "%MO2_SRC_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_SRC_PATH=%MO2_SRC_PATH%"
if not "%QT_ROOT%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DQT_ROOT=%QT_ROOT%"

"%CMAKE_EXE%" .. %CMAKE_ARGS%
if errorlevel 1 goto error_cmake

echo.
echo Building with Ninja...
"%NINJA_EXE%"
if errorlevel 1 goto error_ninja

echo.
echo Build completed successfully!
echo.
echo ========================================
echo Deploying to MO2 plugins folder...
echo ========================================

if "%MO2_PLUGINS_DIR%"=="" (
  echo ERROR: MO2_PLUGINS_DIR is not set. Use config\local.env.bat or an environment variable.
  pause
  exit /b 1
)

if not exist "%MO2_PLUGINS_DIR%" (
	echo ERROR: MO2 plugins folder not found: %MO2_PLUGINS_DIR%
	pause
	exit /b 1
)

if not exist "bin\Release\plugins" (
	echo ERROR: Build output folder not found: bin\Release\plugins
	pause
	exit /b 1
)

copy /Y "bin\Release\plugins\game_*.dll" "%MO2_PLUGINS_DIR%\"
if %ERRORLEVEL% neq 0 (
	echo ERROR: Failed to copy DLLs to %MO2_PLUGINS_DIR%
	pause
	exit /b 1
)

echo ✓ Deployment completed
pause
exit /b 0

:error_cd
echo ERROR: Failed to change to project directory
pause
exit /b 1

:error_vcvars
echo ERROR: Failed to initialize Visual Studio environment
pause
exit /b 1

:error_build_cd
echo ERROR: Failed to change to build directory
pause
exit /b 1

:error_cmake
echo ERROR: CMake configuration failed
pause
exit /b 1

:error_ninja
echo ERROR: Ninja build failed
pause
exit /b 1

endlocal
