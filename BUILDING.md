# Building

This project is set up for team use:
- Shared scripts are committed.
- Machine-specific paths live only in ignored local env files.

## Prerequisites

- Windows 10 or Windows 11 (x64)
- Visual Studio 2022 Build Tools with the C++ workload
- CMake 3.16 or newer
- Ninja
- Qt 6.7.1 (MSVC 2019 64-bit)
- vcpkg
- Mod Organizer 2 uibase and Mod Organizer 2 source tree

## One-Time Local Setup

1. Copy `config/local.env.example.bat` to `config/local.env.bat`.
2. Fill in your local paths in `config/local.env.bat`.
3. Do not edit shared build scripts for personal paths.

`config/local.env.bat` is ignored by git.

## Build

From repository root:

```bat
build_ms.bat
```

`build_ms.bat` reads `config/local.env.bat` automatically if it exists.

## Optional

Install/refresh vcpkg dependencies:

```bat
run_vcpkg_install.bat
```

Build with CMake presets:

```bat
cmake --preset default
cmake --build --preset default
```

## Notes

- Keep personal paths in `config/local.env.bat` only.
- Do not commit machine-specific values.
