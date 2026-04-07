@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "LOCAL_ENV=%SCRIPT_DIR%\config\local.env.bat"
if exist "%LOCAL_ENV%" call "%LOCAL_ENV%"
set "TARGET_ENV=%SCRIPT_DIR%\config\resolve-target-env.bat"
if exist "%TARGET_ENV%" call "%TARGET_ENV%"

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

if not "%VCVARS_BAT%"=="" (
    call "%VCVARS_BAT%" -arch=x64
    if errorlevel 1 goto error_vcvars
)

cd /d "%SCRIPT_DIR%"
if errorlevel 1 goto error_cd

if "%BUILD_DIR%"=="" set "BUILD_DIR=build"

if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    if exist "%BUILD_DIR%" goto error_build_busy
)
mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"
if errorlevel 1 goto error_build_cd

echo.
echo ==========================================
echo Configuring with CMake...
echo ==========================================
set "CMAKE_ARGS=-G Ninja -DCMAKE_BUILD_TYPE=Release"
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
"%CMAKE_EXE%" --build . --config Release --parallel
if errorlevel 1 goto error_ninja

echo.
echo.
echo ==========================================
echo SUCCESS: Build completed
echo ==========================================
echo Build output: %BUILD_DIR%\bin\Release\plugins\
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
echo Make sure no terminal, Explorer window, or process is using files under %SCRIPT_DIR%\%BUILD_DIR%
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
