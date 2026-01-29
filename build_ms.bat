@echo off
setlocal enabledelayedexpansion

REM Auto-detect paths using environment variables and standard locations
set "CMAKE_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=C:\vcpkg"
set "VCPKG_TOOLCHAIN=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

REM Use environment variable or fallback to common download location
REM Standard MO2 plugin convention: uibase in parent directory or MO2Install path
if "%MO2_INSTALL_PATH%"=="" (
    REM Try parent directory first (standard plugin structure)
    set "MO2_LIB_DIR=%SCRIPT_DIR%\..\..\..\uibase"
    if not exist "!MO2_LIB_DIR!" (
        REM Fallback to Downloads location
        set "MO2_LIB_DIR=%USERPROFILE%\Downloads\Mod.Organizer-2.5.2-uibase"
    )
) else (
    set "MO2_LIB_DIR=%MO2_INSTALL_PATH%\uibase"
)

REM SDK typically same location as uibase or specified separately
if "%MO2_SDK_DIR%"=="" set "MO2_SDK_DIR=%MO2_LIB_DIR%"

REM Get script directory
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 goto error_vcvars

cd /d "%SCRIPT_DIR%"
if errorlevel 1 goto error_cd

if exist build rmdir /s /q build
mkdir build
cd build
if errorlevel 1 goto error_build_cd

echo.
echo ==========================================
echo Configuring with CMake...
echo ==========================================
"%CMAKE_EXE%" .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" -DMO2_SDK_DIR="%MO2_SDK_DIR%" -DMO2_LIB_DIR="%MO2_LIB_DIR%"
if errorlevel 1 goto error_cmake

echo.
echo ==========================================
echo Building with Ninja...
echo ==========================================
"%NINJA_EXE%"
if errorlevel 1 goto error_ninja

echo.
echo ==========================================
echo Deploying DLL...
echo ==========================================
set "BIN_DLL=%SCRIPT_DIR%\bin\game_redguard.dll"
set "TARGET_DIR=%SCRIPT_DIR%\..\.."
if "%TARGET_DIR%"=="" goto error_copy
if not exist "%BIN_DLL%" goto error_dll_not_found
copy /Y "%BIN_DLL%" "%TARGET_DIR%\"
if errorlevel 1 goto error_copy

echo.
echo ==========================================
echo SUCCESS: Build and deployment completed
echo ==========================================
echo Deployed to: %TARGET_DIR%\game_redguard.dll
echo.
echo Next steps:
echo 1. Restart ModOrganizer 2
echo 2. Test with a mod archive that has nested packaging
echo 3. MO2 should now offer to auto-fix the structure
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

:error_cmake
echo ERROR: CMake configuration failed
pause
exit /b 1

:error_ninja
echo ERROR: Ninja build failed
pause
exit /b 1

:error_dll_not_found
echo ERROR: DLL was not created during build
pause
exit /b 1

:error_copy
echo ERROR: Failed to copy DLL to plugins folder
pause
exit /b 1

endlocal