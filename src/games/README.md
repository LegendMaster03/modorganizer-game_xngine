# XnGine Game Plugins - Per-Game Module Structure

This directory contains game-specific implementations for XnGine games in Mod Organizer 2.

## File Structure Pattern

Each game plugin follows this naming and structure convention (based on Gamebryo/Skyrim pattern):

```
games/<game>/
├── CMakeLists.txt                 # Build configuration
├── game<game>.h                   # Game plugin header (IPluginGame interface)
├── game<game>.cpp                 # Game plugin implementation
├── game<game>.json                # Plugin metadata
├── <game>moddatachecker.h         # Mod validation (inherits from XngineModDataChecker)
├── <game>moddatacontent.h         # Content categorization (inherits from XngineModDataContent)
├── <game>moddatacontent.cpp       # Content implementation
├── <game>savegame.h               # Save game handling (inherits from XngineSaveGame)
└── <game>savegame.cpp             # Save game implementation
```

### Example: Redguard

```
games/redguard/
├── gameredguard.h                 # GameRedguard : GameXngine
├── gameredguard.cpp
├── gameredguard.json
├── redguardsmoddatachecker.h      # RedguardsModDataChecker : XngineModDataChecker
├── redguardsmoddatacontent.h      # RedguardsModDataContent : XngineModDataContent
├── redguardsmoddatacontent.cpp
├── redguardsavegame.h             # RedguardsSaveGame : XngineSaveGame
└── redguardsavegame.cpp
```

## Implementation Guidelines

### Game Plugin Header (game<game>.h)

```cpp
class Game<Game> : public GameXngine
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org.tannin.Game<Game>" FILE "game<game>.json")

public:
  Game<Game>();
  virtual bool init(MOBase::IOrganizer* moInfo) override;

public:  // Override game-specific methods
  virtual QString gameName() const override;
  virtual QList<MOBase::ExecutableInfo> executables() const override;
  virtual QString steamAPPId() const override;
  // ... other overrides
  
protected:
  virtual QString identifyGamePath() const override;  // Game detection logic
  virtual QDir savesDirectory() const override;        // Game-specific save path
};
```

### Implementation (game<game>.cpp)

Key methods to implement:

1. **`identifyGamePath()`** - Detect game installation directory
   - Search known Steam/GOG paths
   - Look for game-specific markers (executables, config files)
   - Return empty string if not found

2. **`savesDirectory()`** - Return path to save game folder
   - Different between Steam and GOG versions
   - For Redguard: Redguard/SAVEGAME/ (Steam) or SAVE/ (GOG)

3. **`gameName()`** - Display name in MO2
4. **`executables()`** - List of executable variants
5. **`steamAPPId()`** - Steam app ID (if applicable)
6. **`gogAPPId()`** - GOG app ID (if applicable)
7. **`init()`** - Initialize game features (data checker, content categorizer, etc.)

### Mod Data Checker (<game>moddatachecker.h)

```cpp
class <Game>sModDataChecker : public XngineModDataChecker
{
protected:
  virtual const FileNameSet& possibleFolderNames() const override
  {
    static FileNameSet result{"folder1", "folder2", ...};
    return result;
  }
  
  virtual const FileNameSet& possibleFileExtensions() const override
  {
    static FileNameSet result{"ext1", "ext2", ...};
    return result;
  }
};
```

Folder names and extensions are game-specific. For Redguard:
- Folders: data, maps, textures, sounds, etc.
- Extensions: rgm, rtx, dat, mif, img, pal, fnt, ini, txt, cfg

### Mod Data Content (<game>moddatacontent.h)

Inherits from `XngineModDataContent`. In most cases, no override needed.
Override `possibleFolderNames()` or `possibleFileExtensions()` if game has unique patterns.

### Save Game Handler (<game>savegame.h)

Inherits from `XngineSaveGame`. Most logic is inherited. Add game-specific save parsing if needed.

## Building

To build all game plugins, the main CMakeLists.txt in src/ will include subdirectories:

```cmake
add_subdirectory(games/redguard)
add_subdirectory(games/daggerfall)
add_subdirectory(games/battlespire)
```

Each game plugin generates a separate DLL in build/Release/

## Registration

In `game<game>.cpp::init()`, register game features:

```cpp
registerFeature(std::make_shared<<Game>sModDataChecker>(this));
registerFeature(std::make_shared<<Game>sModDataContent>(m_Organizer->gameFeatures()));
registerFeature(std::make_shared<XngineSaveGameInfo>(this));
registerFeature(std::make_shared<XngineLocalSavegames>(this));
registerFeature(std::make_shared<XngineUnmanagedMods>(this));
```

## Game-Specific Notes

### Redguard
- Detection: Look for DOSBox-0.73/ (Steam) or DOSBOX/ (GOG)
- Saves: SAVEGAME.XXX folders containing SAVEGAME.SAV
- Executables: DOSBox wrapper + Redguard.exe
- Mod formats: Format 0 (.RGM/.RTX files) and Format 1 (About.txt + Changes.txt)

### Daggerfall (WIP)
- Detection: DAGGER.EXE or ARENA2/ directory
- Saves: SAVE0-SAVE5 at root or DF/DAGGER/
- Mod formats: Similar to Redguard

### Arena (WIP)
- Detection: GLOBAL.BSA or Arena-specific markers
- Saves: Game-specific
- Mod formats: To be determined

## Integration with XnGine Core

The game plugins should NOT re-implement engine-level logic. Instead:

1. Use `GameXngine` base class for common functionality
2. Override only game-specific methods (path detection, save directory, executables)
3. Use `XngineModDataChecker`, `XngineSaveGameInfo`, etc. from xngine/
4. Create game-specific subclasses only when game has unique requirements

This keeps logic clean and maintainable across multiple XnGine games.
