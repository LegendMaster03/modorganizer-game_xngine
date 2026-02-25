@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%

set "VSCMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "VSNINJA=C:\PROGRA~2\MICROS~2\2022\BUILDT~1\Common7\IDE\COMMON~1\MICROS~1\CMake\Ninja\ninja.exe"

"%VSCMAKE%" -S tools\bsa_extract_cli -B build\bsa_extract_cli -G Ninja -DCMAKE_MAKE_PROGRAM=%VSNINJA% -DCMAKE_C_COMPILER="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" -DCMAKE_CXX_COMPILER="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" -DQt6_DIR=C:\Qt\6.7.1\msvc2019_64\lib\cmake\Qt6
if errorlevel 1 exit /b %errorlevel%

"%VSCMAKE%" --build build\bsa_extract_cli --config Release
exit /b %errorlevel%
