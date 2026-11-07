# XnGine Game Plugins for Mod Organizer 2

This repository provides Mod Organizer 2 (MO2) game plugins for Bethesda XnGine titles. The plugins share a C++/Qt XnGine base while keeping game-specific detection, data formats, and save handling in separate modules.

## Game modules

- The Elder Scrolls: Arena
- An Elder Scrolls Legend: Battlespire
- The Elder Scrolls II: Daggerfall
- The Elder Scrolls Adventures: Redguard

The common integration is in `src/xngine/`; per-game modules are in `src/games/arena/`, `src/games/battlespire/`, `src/games/daggerfall/`, and `src/games/redguard/`. Support for particular archive variants, save formats, and game installations depends on each module's implementation and should be verified with representative game data.

## Build requirements

- Windows 10/11 (x64)
- Visual Studio 2022 Build Tools (Desktop development with C++)
- CMake 3.16+ and Ninja
- Qt 6.7.1 (MSVC 2019 64-bit)
- vcpkg (including zlib and lz4)
- A compatible Mod Organizer 2 SDK / UIBase build

See [BUILDING.md](BUILDING.md) for detailed setup. Dependencies are also recorded in `vcpkg.json` and `CMakePresets.json`.

## Build and deploy

1. Copy `config/local.env.example.bat` to `config/local.env.bat` and configure local dependency and MO2 paths.
2. Build and deploy to the configured MO2 plugins directory:

```bat
build_and_deploy.bat
```

The deployment target comes from your environment or local configuration. Validate the game plugin in an MO2 installation with the corresponding game data. The repository includes `check_logs.bat` for local log inspection. There is no `TESTING_GUIDE.md` in the current tree; follow [BUILDING.md](BUILDING.md) and relevant source and script documentation instead.

## Technical documentation

The following guides explain current implementation contracts, reverse-engineered
format assumptions, and known limitations. They do not imply complete game-data
coverage or verified binary round trips.

- [Shared BSA archive contracts](docs/xngine-archive-contracts.md)
- [Executable patch staging and validation boundaries](docs/executable-patching-contracts.md)
- [Redguard generated patch lifecycle](docs/redguard-patch-lifecycle.md)
- [Daggerfall BLOCKS/MAPS, 3D meshes, and Redguard map data](docs/binary-format-interpretation.md)
- [WLD, RSC, palettes, images, and other resources](docs/resource-format-interpretation.md)
- [Quest and script decoding](docs/quest-and-script-interpretation.md)
- [Mod package detection and installation roots](docs/mod-package-detection.md)
- [MO2 profile adapters and archive-list helpers](docs/mo2-profile-adapters.md)

## Licensing and provenance

There is currently no top-level `LICENSE` file in this repository. Public source availability does not itself establish redistribution rights; verify the applicable licenses and permissions for original and reused components before redistributing them.

The Redguard integration draws on earlier Redguard Mod Manager Java work, adapted for C++ and MO2. The MO2 plugin system and associated libraries belong to their respective maintainers.

## Credits

- Dillonn241 for early Redguard modding work
- The Mod Organizer 2 team for the plugin system
- Bethesda for the XnGine games

For questions, issues, or contributions, use this repository's GitHub issue tracker.
