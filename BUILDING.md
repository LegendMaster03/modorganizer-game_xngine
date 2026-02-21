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

## Advanced (Developers)

### Daggerfall Build Profiles

The Daggerfall plugin supports keep/prune profiles via CMake options.

- `daggerfall-runtime`: core runtime only (no toolkit, no EXE patching)
- `daggerfall-toolkit`: runtime + toolkit (default safe advanced profile)
- `daggerfall-full`: toolkit + EXE patching (explicit legacy/unsafe profile)

Use these presets:

```bat
cmake --preset daggerfall-runtime
cmake --build --preset daggerfall-runtime
```

```bat
cmake --preset daggerfall-toolkit
cmake --build --preset daggerfall-toolkit
```

```bat
cmake --preset daggerfall-full
cmake --build --preset daggerfall-full
```

## Notes

- Keep personal paths in `config/local.env.bat` only.
- Do not commit machine-specific values.
