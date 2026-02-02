@echo off
setlocal enabledelayedexpansion

set "CMAKE_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "VCPKG_TOOLCHAIN=C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
set "MO2_LIB_DIR=C:\Users\kbarn\Downloads\Mod.Organizer-2.5.2-uibase"
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

cd /d "%~dp0"
if errorlevel 1 goto error_cd

call "%VCVARS%"
if errorlevel 1 goto error_vcvars

if exist build rmdir /s /q build
mkdir build
cd build
if errorlevel 1 goto error_build_cd

echo Configuring CMake...
"%CMAKE_EXE%" .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" -DMO2_LIB_DIR="%MO2_LIB_DIR%"
if errorlevel 1 goto error_cmake

echo.
echo Building with Ninja...
"%NINJA_EXE%"
if errorlevel 1 goto error_ninja

echo.
echo Build completed successfully!
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
