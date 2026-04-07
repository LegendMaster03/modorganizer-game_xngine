@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" %*
if errorlevel 1 exit /b %errorlevel%

set "VCPKG_ROOT=C:\vcpkg"
set "VCPKG_VISUAL_STUDIO_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
set "CC=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
set "CXX=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
