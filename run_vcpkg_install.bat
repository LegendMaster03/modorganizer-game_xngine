@echo off
set PATH=%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem;%SystemRoot%\System32\WindowsPowerShell\v1.0%
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set VCPKG_FORCE_SYSTEM_BINARIES=
set VCPKG_ROOT=C:/vcpkg
set CC=cl
set CXX=cl
echo VCPKG_ROOT=%VCPKG_ROOT%
cd /d C:\vcpkg
"C:/vcpkg/vcpkg.exe" install qtbase:x64-windows zlib:x64-windows
