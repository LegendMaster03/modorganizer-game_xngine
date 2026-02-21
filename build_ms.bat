@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "LOCAL_ENV=%SCRIPT_DIR%\config\local.env.bat"
if exist "%LOCAL_ENV%" call "%LOCAL_ENV%"

if "%CMAKE_EXE%"=="" set "CMAKE_EXE=cmake"
if "%VCPKG_ROOT%"=="" (
    set "VCPKG_TOOLCHAIN="
) else (
    set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

if not "%VCVARS_BAT%"=="" (
    call "%VCVARS_BAT%"
    if errorlevel 1 goto error_vcvars
)

cd /d "%SCRIPT_DIR%"
if errorlevel 1 goto error_cd

if exist build (
    rmdir /s /q build
    if exist build goto error_build_busy
)
mkdir build
cd build
if errorlevel 1 goto error_build_cd

echo.
echo ==========================================
echo Configuring with CMake...
echo ==========================================
set "CMAKE_ARGS=-G Ninja -DCMAKE_BUILD_TYPE=Release"
if not "%NINJA_EXE%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_MAKE_PROGRAM=""%NINJA_EXE%"""
if not "%VCPKG_TOOLCHAIN%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE=""%VCPKG_TOOLCHAIN%"""
if not "%MO2_UIBASE_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_PATH=""%MO2_UIBASE_PATH%"""
if not "%MO2_UIBASE_LIB%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_UIBASE_LIB=""%MO2_UIBASE_LIB%"""
if not "%MO2_SRC_PATH%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DMO2_SRC_PATH=""%MO2_SRC_PATH%"""
if not "%QT_ROOT%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DQT_ROOT=""%QT_ROOT%"" -DQt6_DIR=""%QT_ROOT%\lib\cmake\Qt6"""
"%CMAKE_EXE%" .. %CMAKE_ARGS%
if errorlevel 1 goto error_cmake

echo.
echo ==========================================
echo Building with Ninja...
echo ==========================================
"%NINJA_EXE%"
if errorlevel 1 goto error_ninja

echo.
echo.
echo ==========================================
echo SUCCESS: Build completed
echo ==========================================
echo Build output: build\bin\Release\plugins\
echo.
pause
exit /b 0

:error_vcvars
echo ERROR: Failed to initialize Visual Studio environment
pause
exit /b 1

:error_cd
echo ERROR: Failed to change to project directory
pause
exit /b 1

:error_build_cd
echo ERROR: Failed to change to build directory
pause
exit /b 1

:error_build_busy
echo ERROR: Failed to remove existing build directory.
echo Make sure no terminal, Explorer window, or process is using files under %SCRIPT_DIR%\build
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
