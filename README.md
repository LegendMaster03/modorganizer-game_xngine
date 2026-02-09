# XnGine Game Plugins for Mod Organizer 2

This repository provides Mod Organizer 2 (MO2) game plugins for Bethesda XnGine titles. The plugins share a common XnGine base and expose per-game modules.

## Status

Work in progress. Builds succeed, but MO2 crashes during plugin metadata parsing (Qt6 `qt_plugin_query_metadata_v2`), so the plugins do not currently load in MO2.

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

## Build and Deploy

Build and deploy to your MO2 plugins folder:

```bat
build_and_deploy.bat
```

Deploy only (if already built):

```powershell
.\deploy_only.ps1
```

Default target path is `C:\Modding\MO2\plugins` (adjust in scripts as needed).

## Current Crash (Known Blocker)

MO2 crashes while loading `game_battlespire.dll` during Qt plugin metadata parsing.

- Call stack: `Qt6Core!QtPrivate::QStringList_join` in `qt_plugin_query_metadata_v2`
- Occurs before plugin instantiation
- Repro: launch MO2 with any of the XnGine plugins present in `plugins/`

### What Has Been Tried

- `Q_PLUGIN_METADATA` with and without JSON
- Standard MO2 IID: `com.tannin.ModOrganizer.PluginGame/2.0`
- `Q_INTERFACES` moved to concrete plugin classes
- Removed duplicate `QObject` base
- Simplified MOC inputs and metadata

## Repo Layout

```
src/
  xngine/        Shared engine implementation
  games/
    battlespire/ Game module
    daggerfall/  Game module
    redguard/    Game module
build_and_deploy.bat
CMakeLists.txt
```

## License

This plugin is open source. See individual source files for licensing details.

## Credits

Based on the Redguard Mod Manager Java implementation. Ported to C++ for MO2 integration.

## Support

For issues, questions, or contributions, see the development documentation in `docs/archive/`.
3. Follow build instructions above
4. Test thoroughly using [TESTING_GUIDE.md](TESTING_GUIDE.md)

## 📄 License

See project license file for details.

## 🙏 Acknowledgments

- Dillonn241 for starting the Redguard modding scene
- ModOrganizer 2 team for the plugin system
- Bethesda for the XnGine engine and Redguard
