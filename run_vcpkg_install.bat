@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "LOCAL_ENV=%SCRIPT_DIR%\config\local.env.bat"
if exist "%LOCAL_ENV%" call "%LOCAL_ENV%"

if "%VCPKG_ROOT%"=="" (
	echo ERROR: VCPKG_ROOT is not set. Use config\local.env.bat or an environment variable.
	exit /b 1
)

if not "%VCVARS_BAT%"=="" (
	call "%VCVARS_BAT%"
	if errorlevel 1 goto error_vcvars
)

set VCPKG_FORCE_SYSTEM_BINARIES=
set CC=cl
set CXX=cl

echo VCPKG_ROOT=%VCPKG_ROOT%
pushd "%SCRIPT_DIR%"
"%VCPKG_ROOT%\vcpkg.exe" install
popd
exit /b 0

:error_vcvars
echo ERROR: Failed to initialize Visual Studio environment
exit /b 1
