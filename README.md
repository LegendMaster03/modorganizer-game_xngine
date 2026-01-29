# Redguard MO2 Plugin

MO2 (Mod Organizer 2) game plugin for **The Elder Scrolls Adventures: Redguard**.

Enables mod management for both modern file-replacement mods (Type 2) and legacy patch-based mods (Type 1) from the original Redguard Mod Manager.

## Features

✅ **Type 1 Mods** (Redguard Mod Manager format)
- INI Changes (menu/config modifications)
- Map Changes (script/map modifications)
- RTX Changes (dialogue/text edits)
- Audio file replacements
- Texture file replacements

✅ **Type 2 Mods** (Modern VFS format)
- Direct file replacement via MO2's virtual file system
- Full compatibility with standard MO2 mod archives

✅ **Auto-Fix** for nested mod archives

## Quick Start

### Building
```bash
.\build_ms.bat
```

### Installation
Plugin DLL automatically deploys to MO2 plugins folder. Restart MO2 to load.

### Usage
1. Launch MO2 with Redguard selected as the game
2. Enable mods in the left panel
3. Click "Run" to launch the game
4. Mods are automatically applied via VFS overlay

## Documentation

- **[MODFORMAT.md](MODFORMAT.md)** - Mod format specifications
- **[docs/archive/](docs/archive/)** - Development notes and guides

## ✅ Feature Status

| Feature | Status |
|---------|--------|
| Type 1 Mods (Patch-based) | ✅ **COMPLETE** |
| Type 2 Mods (File Replacement) | ✅ Complete |
| INI Changes | ✅ Complete |
## Requirements

- Windows (64-bit)
- Mod Organizer 2 (2.5.0+)
- The Elder Scrolls Adventures: Redguard (GOG or CD)
- Visual Studio 2019/2022 with C++ (for building)
- CMake 3.16+
- Qt 6.7.1

## Building from Source

### Prerequisites
1. Install Visual Studio 2019/2022 with C++ Desktop Development
2. Install CMake 3.16 or later
3. Install Qt 6.7.1 (MSVC 2019 64-bit)

### Build Steps
```bash
cd modorganizer-game_redguard
.\build_ms.bat
```

Output: `F:\Modding\MO2\plugins\game_redguard.dll`

## Project Structure

```
modorganizer-game_redguard/
├── src/                       # C++ source files
│   ├── gameredguard.cpp      # Main plugin entry point
│   ├── RGMODFrameworkWrapper.* # Type 1/2 mod loading
│   ├── RtxDatabase.*         # Dialogue/text database
│   ├── MapFile.*             # Map binary parser
│   └── ...                   # Supporting classes
├── docs/archive/             # Development documentation
├── CMakeLists.txt            # Build configuration
└── build_ms.bat              # Build script
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
