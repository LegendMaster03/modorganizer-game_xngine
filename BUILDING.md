# Building

These instructions follow the same level of detail as modorganizer-game_bethesda, but use local environment variables so no private paths are required in the repository.

## Prerequisites

- Windows 10 or Windows 11 (x64)
- Visual Studio 2022 Build Tools with the C++ workload
- CMake 3.16 or newer
- Ninja
- Qt 6.7.1 (MSVC 2019 64-bit)
- vcpkg
- Mod Organizer 2 SDK (uibase) and Mod Organizer 2 source tree

## Configure Local Environment

1. Copy config/local.env.example.bat to config/local.env.bat.
2. Fill in the variables for your machine:
   - MO2_PLUGINS_DIR
   - MO2_UIBASE_PATH
   - MO2_UIBASE_LIB
   - MO2_SRC_PATH
   - QT_ROOT
   - VCPKG_ROOT
   - VCVARS_BAT

If you prefer PowerShell, copy config/local.env.example.ps1 to config/local.env.ps1 instead.

## Install Dependencies

This repository uses vcpkg manifest mode. From the repository root:

```bat
run_vcpkg_install.bat
```

## Build With CMake Presets

```bat
cmake --preset default
cmake --build --preset default
```

## Build and Deploy

```bat
build_and_deploy.bat
```

## Deploy Only

```powershell
.\deploy_only.ps1
```

## Notes

- CMake uses the environment variables provided by config/local.env.bat or config/local.env.ps1.
- The repository does not store any machine specific paths.
