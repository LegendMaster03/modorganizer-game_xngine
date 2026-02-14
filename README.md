# XnGine Game Plugins for Mod Organizer 2

This repository provides Mod Organizer 2 (MO2) game plugins for Bethesda XnGine titles. The plugins share a common XnGine base and expose per-game modules.

## Supported Games

- An Elder Scrolls Legend: Battlespire
- The Elder Scrolls II: Daggerfall
- The Elder Scrolls Adventures: Redguard

## Build Requirements

- Windows 10/11 (x64)
- Visual Studio 2022 Build Tools (C++ Desktop workload)
- CMake 3.16+
- Ninja
- Qt 6.7.1 (MSVC 2019 64-bit)
- vcpkg (zlib, lz4)
- Mod Organizer 2 SDK (uibase)

Dependencies are described in vcpkg.json. You can use CMakePresets.json with environment variables for local paths.

## Building

See [BUILDING.md](BUILDING.md) for full setup and build instructions.

## Build and Deploy

1. Copy `config/local.env.example.bat` to `config/local.env.bat` and fill in your paths.
2. If you prefer PowerShell, copy `config/local.env.example.ps1` to `config/local.env.ps1`.

Build and deploy to your MO2 plugins folder:

```bat
build_and_deploy.bat
```

Deploy only (if already built):

```powershell
.\deploy_only.ps1
```

Default target path is your MO2 plugins directory (set via environment or local config).

## License

This plugin is open source. See individual source files for licensing details.

## Credits

Based on the Redguard Mod Manager Java implementation. Ported to C++ for MO2 integration.

## Support

For issues or questions, open a GitHub issue or ask in the Mod Organizer 2 community.

## Acknowledgments

- Dillonn241 for starting the Redguard modding scene
- Mod Organizer 2 team for the plugin system
- Bethesda for the XnGine engine and Redguard

## Support

For issues, questions, or contributions, see the development documentation in `docs/archive/`.
3. Follow build instructions above
4. Test thoroughly using [TESTING_GUIDE.md](TESTING_GUIDE.md)

## 📄 License

See project license file for details.

## 🙏 Acknowledgments

- Dillonn241 for starting the Redguard modding scene
- ModOrganizer 2 team for the plugin system
- Bethesda for XnGine
