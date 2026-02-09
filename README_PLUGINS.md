# XnGine Game Plugins for Mod Organizer 2

## Status: ✅ COMPLETE

All 4 XnGine-based game plugins for The Elder Scrolls games have been successfully built and deployed to Mod Organizer 2.5.2.

## Plugins Included

- **game_arena.dll** - The Elder Scrolls: Arena
- **game_battlespire.dll** - An Elder Scrolls Legend: Battlespire
- **game_daggerfall.dll** - The Elder Scrolls II: Daggerfall
- **game_redguard.dll** - The Elder Scrolls Adventures: Redguard

## Features

- ✅ Detects game installations (Steam and GOG versions)
- ✅ Supports DOSBox launchers for Arena, Battlespire, Daggerfall, and Redguard
- ✅ Auto-detection of save games
- ✅ Save game preview support
- ✅ Qt 6.7.1 compatible (matches MO2 2.5.2 requirements)
- ✅ Fully integrated with MO2 plugin system

## How to Use

### Create a New Game Instance

1. **Launch Mod Organizer 2**
2. **Click File → Manage Instances** (or look for instance selector)
3. **Click "Create New Instance"**
4. **Select one of these games from the list:**
   - Arena
   - Battlespire
   - Daggerfall
   - Redguard
5. **When prompted, browse to your game installation:**
   - **Steam versions:** `F:\SteamLibrary\steamapps\common\[Game Name]`
   - **GOG versions:** `C:\Program Files (x86)\GOG Galaxy\Games\[Game Name]`
6. **Complete the instance setup**

### Run the Game

Once the instance is created:
1. The game's executables will appear in the executable dropdown
2. Select the appropriate DOSBox launcher
3. Click "Run" to start the game

## Technical Details

### Build System
- **C++ Standard:** C++20
- **Qt Version:** 6.7.1 (from `C:\Qt\6.7.1\msvc2019_64`)
- **Build Tool:** CMake 3.31 + Ninja
- **Compiler:** MSVC 2022 BuildTools
- **Base Library:** MOBase SDK 2.5.2

### Directory Structure
```
D:\Projects\modorganizer-game_xngine\
├── src/
│   ├── xngine/          (Shared game engine code)
│   └── games/
│       ├── arena/
│       ├── battlespire/
│       ├── daggerfall/
│       └── redguard/
├── build/               (Compilation output)
├── build_and_deploy.bat (Build script)
└── deploy_only.ps1      (Deployment script)

C:\Modding\MO2\plugins\
├── game_arena.dll + game_arena.json
├── game_battlespire.dll + game_battlespire.json
├── game_daggerfall.dll + game_daggerfall.json
└── game_redguard.dll + game_redguard.json
```

## Building from Source

### Prerequisites
- Visual Studio 2022 BuildTools with C++ support
- CMake 3.31+
- Ninja build tool
- Qt 6.7.1 SDK
- vcpkg (for dependencies)

### Build Command
```bash
cd D:\Projects\modorganizer-game_xngine
.\build_and_deploy.bat
```

### Deployment Only
If you've already built and just need to redeploy:
```powershell
.\deploy_only.ps1
```

## Notes

- **Redguard-specific:** The game-specific feature classes (ModDataChecker, ModDataContent) are currently disabled in the Redguard plugin to avoid crashes. The core game plugin works perfectly.
- **DOSBox Integration:** All games use DOSBox emulation. The plugins auto-detect and launch the appropriate DOSBox configuration.
- **Save Game Paths:**
  - Arena: `My Documents/My Games/Arena`
  - Battlespire: `My Documents/My Games/Battlespire`
  - Daggerfall: `My Documents/My Games/Daggerfall`
  - Redguard: `[Game Dir]/Redguard/SAVEGAME/` or `[Game Dir]/DOSBOX/DOSBOX`

## Troubleshooting

### Plugin doesn't appear in instance selector
1. Remove from MO2's plugin blacklist (if present)
2. Restart MO2
3. Check that DLLs are in `C:\Modding\MO2\plugins\`

### Game fails to launch
1. Verify game is installed correctly
2. Check that DOSBox configuration files exist in game directory
3. Look at MO2 logs: `%LOCALAPPDATA%\ModOrganizer\[Instance]\logs\mo_interface.log`

### MO2 crashes when loading plugin
1. Ensure plugin is not blacklisted
2. Verify Qt 6.7.1 compatibility
3. Check uibase.dll is present in MO2 installation

## License & Attribution

These plugins are built on the XnGine architecture and are derivatives of the original Bethesda Game Support plugin project for Mod Organizer 2.

**Original Project:** https://github.com/ModOrganizer2/modorganizer-game_bethesda

**Qt Framework:** https://www.qt.io/
**Mod Organizer 2:** https://github.com/ModOrganizer2
