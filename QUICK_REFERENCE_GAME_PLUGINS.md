# Quick Reference: Creating XnGine Game Plugins

## Template Checklist

When creating a new XnGine game plugin (e.g., Daggerfall), use this checklist:

### 1. Create Directory
```bash
mkdir src/games/<game>
```

### 2. Create Game Plugin Class (game<game>.h)
- [ ] Inherit from GameXngine
- [ ] Add Q_OBJECT and Q_PLUGIN_METADATA macros
- [ ] Declare pure virtual method overrides:
  - gameName(), gameShortName(), gameNexusName()
  - steamAPPId(), binaryName()
  - executables()
  - nexusGameID(), nexusModOrganizerID()
  - name(), localizedName(), author(), description(), version(), settings()
- [ ] Protected overrides:
  - identifyGamePath()
  - savesDirectory()

### 3. Implement Game Plugin (game<game>.cpp)
- [ ] Include all needed xngine/ headers
- [ ] Implement init() - register all features
- [ ] Implement identifyGamePath() - search for game installation
- [ ] Implement savesDirectory() - return save folder path
- [ ] Implement executables() - return list of launch options
- [ ] Implement other IPluginGame methods with game-specific info

### 4. Create Mod Data Checker (<game>moddatachecker.h)
- [ ] Inherit from XngineModDataChecker
- [ ] Override possibleFolderNames() with game-specific folder names
- [ ] Override possibleFileExtensions() with game-specific extensions

### 5. Create Mod Data Content (<game>moddatacontent.h/cpp)
- [ ] Inherit from XngineModDataContent
- [ ] Minimal implementation (usually just inherits defaults)

### 6. Create Save Game Handler (<game>savegame.h/cpp)
- [ ] Inherit from XngineSaveGame
- [ ] Minimal implementation (usually just inherits defaults)
- [ ] Add game-specific save parsing only if needed

### 7. Create CMakeLists.txt
- [ ] Copy from redguard example
- [ ] Update project name and source files
- [ ] Ensure target name is game_<game>

### 8. Create game<game>.json Metadata
```json
{
  "name": "Game Name Support Plugin",
  "author": "Your Name",
  "version": "1.0.0",
  "description": "Adds support for ...",
  "url": "",
  "requirements": []
}
```

### 9. Update Main CMakeLists.txt
- [ ] Add `add_subdirectory(games/<game>)` in src/CMakeLists.txt
- [ ] Test compilation

### 10. Verify Files
```
games/<game>/
├── CMakeLists.txt
├── game<game>.h
├── game<game>.cpp
├── game<game>.json
├── <game>moddatachecker.h
├── <game>moddatacontent.h
├── <game>moddatacontent.cpp
├── <game>savegame.h
└── <game>savegame.cpp
```

## Redguard Reference Files

Use these as templates:

| File | Location | Purpose |
|------|----------|---------|
| Game Plugin | `src/games/redguard/gameredguard.h/cpp` | Template for game class |
| Mod Checker | `src/games/redguard/redguardsmoddatachecker.h` | Template for mod validation |
| Mod Content | `src/games/redguard/redguardsmoddatacontent.h/cpp` | Template for content categorization |
| Save Game | `src/games/redguard/redguardsavegame.h/cpp` | Template for save handler |
| CMake | `src/games/redguard/CMakeLists.txt` | Template for build config |

## Key Points

1. **All game detection in identifyGamePath()** - Search known paths, check for game markers
2. **All save directory logic in savesDirectory()** - Handle Steam vs GOG differences
3. **Game-specific mod patterns in <game>moddatachecker.h** - Define folder/extension lists
4. **Minimal game-specific content logic** - Usually just inherit from XngineModDataContent
5. **Register all features in init()** - Tells MO2 what capabilities the plugin supports

## Common Patterns

### Game Path Detection
```cpp
QString GameXYZ::identifyGamePath() const
{
  QStringList searchPaths = {
    // Steam path
    // GOG path
    // Fallback paths
  };
  
  for (const QString& path : searchPaths) {
    QDir dir(path);
    // Check for game markers (exe files, config files, game data)
    if (/* found game */) return path;
  }
  return {};
}
```

### Save Directory Mapping
```cpp
QDir GameXYZ::savesDirectory() const
{
  QDir gameDir = gameDirectory();
  
  // Steam version: saves in SubDir/SAVE/ or SAVEGAME/
  QDir steamSaves(gameDir.filePath("SubDir/SAVES"));
  if (steamSaves.exists()) return steamSaves;
  
  // GOG version: saves in different location
  QDir gogSaves(gameDir.filePath("SAVE"));
  if (gogSaves.exists()) return gogSaves;
  
  return gameDir;  // Fallback
}
```

### Executables List
```cpp
QList<ExecutableInfo> GameXYZ::executables() const
{
  QList<ExecutableInfo> exes;
  
  // Primary game exe
  exes << ExecutableInfo("Game Name",
                         findInGameFolder("GAME.EXE"));
  
  // Alternative launchers
  exes << ExecutableInfo("Launcher",
                         findInGameFolder("Launch.exe"));
  
  return exes;
}
```

## MOBase Include Patterns

```cpp
#include <executableinfo.h>        // For ExecutableInfo
#include <pluginsetting.h>         // For PluginSetting
#include <iplugingame.h>          // For IPluginGame base
#include <versioninfo.h>          // For VersionInfo
```

## Debugging

- Use `qDebug()` for debug output (appears in MO2 logs)
- Use `qWarning()` for warnings
- Use `qCritical()` for errors
- Logs appear in MO2's log directory

## Common Issues

**"Cannot find xngine header"** → Check include paths in CMakeLists.txt
**"Unresolved external symbol"** → Ensure xngine/ library is linked
**"Game not detected"** → Check identifyGamePath() search paths and markers
**"Saves not found"** → Check savesDirectory() returns correct path
**"Plugin fails to load in MO2"** → Check game<game>.json exists and is valid

## Version Info Pattern
```cpp
MOBase::VersionInfo version() const override
{
  return MOBase::VersionInfo(1, 0, 0, MOBase::VersionInfo::RELEASE_FINAL);
}
```

## Nexus IDs

- Redguard: No Nexus support yet (ID: 0)
- Daggerfall: Check if Nexus game ID exists
- Arena: Check if Nexus game ID exists
- Battlespire: Check if Nexus game ID exists

If no Nexus ID, return 0 and document why.
