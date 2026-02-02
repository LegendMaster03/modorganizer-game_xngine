# Project Architecture Overview

Visual guide to the XnGine engine framework and per-game module system.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│  Mod Organizer 2 (MO2)                                              │
│  ├─ IPluginGame Interface                                           │
│  └─ IOrganizer API                                                  │
└─────────────────────┬───────────────────────────────────────────────┘
                      │
                      ↓
┌─────────────────────────────────────────────────────────────────────┐
│  XnGine Engine Framework (src/xngine/)                              │
│  ├─ GameXngine (Base Game Plugin)                                   │
│  │  ├─ Game detection stubs (override in subclass)                  │
│  │  ├─ Directory methods (override in subclass)                     │
│  │  └─ Feature management                                           │
│  │                                                                  │
│  ├─ XngineModDataChecker                                            │
│  │  └─ Generic mod validation with XnGine patterns                  │
│  │                                                                  │
│  ├─ XngineModDataContent                                            │
│  │  └─ Content categorization (PATCH_INSTRUCTIONS, etc.)            │
│  │                                                                  │
│  ├─ XngineSaveGameInfo                                              │
│  │  └─ Save game display widget                                     │
│  │                                                                  │
│  ├─ XngineLocalSavegames                                            │
│  │  └─ Save directory mapping                                       │
│  │                                                                  │
│  └─ 5+ more feature classes                                         │
└─────────────────────┬───────────────────────────────────────────────┘
                      │
        ┌─────────────┼─────────────┬─────────────┬──────────────┐
        ↓             ↓             ↓             ↓              ↓
    ┌────────┐   ┌─────────┐   ┌──────┐   ┌────────────┐   ┌──────┐
    │Redguard│   │Daggerfall│  │Arena │   │Battlespire │  │Others│
    │✅      │   │🟡       │  │🟡    │   │🟡         │  │🟠    │
    └────────┘   └─────────┘   └──────┘   └────────────┘   └──────┘
    (Complete)   (Planned)    (Planned)   (Planned)       (Research)
```

## Per-Game Module Structure

Each game module follows this pattern:

```
src/games/<game>/
│
├─ game<game>.h
│  └─ class Game<Game> : public GameXngine
│     ├─ init()                    ← Register features
│     ├─ identifyGamePath()        ← Game detection
│     ├─ savesDirectory()          ← Save location mapping
│     ├─ executables()             ← Launcher list
│     ├─ gameName(), steamAPPId(), etc.
│     └─ nexusGameID(), nexusModOrganizerID()
│
├─ game<game>.cpp
│  └─ Implementation of all methods
│
├─ <game>smoddatachecker.h
│  └─ class <Game>sModDataChecker : public XngineModDataChecker
│     └─ Game-specific mod patterns (folders, extensions)
│
├─ <game>smoddatacontent.h/cpp
│  └─ class <Game>sModDataContent : public XngineModDataContent
│     └─ Content categorization (usually inherits defaults)
│
├─ <game>savegame.h/cpp
│  └─ class <Game>sSaveGame : public XngineSaveGame
│     └─ Save file handling (usually inherits defaults)
│
├─ game<game>.json
│  └─ Plugin metadata (name, version, description)
│
└─ CMakeLists.txt
   └─ Build configuration for this game module
```

## Class Inheritance Diagram

```
                    IPluginGame (MOBase)
                          ↑
                          │
                    GameXngine (xngine/)
                  (Base for all XnGine games)
                   /    |      |      \
                  /     |      |       \
            Redguard Daggerfall Arena Battlespire
          (games/*)   (games/*)  (games/*) (games/*)
           ✅          🟡        🟡        🟡
```

## Feature Registration Pattern

Each game module registers these features in `init()`:

```
Game<Game>::init(IOrganizer* moInfo)
{
  GameXngine::init(moInfo);  // Initialize base class first
  
  // Register game-specific features
  registerFeature(<Game>sModDataChecker);
  registerFeature(<Game>sModDataContent);
  
  // Register generic XnGine features
  registerFeature(XngineSaveGameInfo);
  registerFeature(XngineLocalSavegames);
  registerFeature(XngineUnmanagedMods);
  
  return true;
}
```

## Build System Structure

```
CMakeLists.txt (root)
│
└─ src/
   ├─ CMakeLists.txt
   │  ├─ Configures xngine/ core
   │  ├─ add_subdirectory(games/redguard)     ✅
   │  ├─ add_subdirectory(games/daggerfall)   🟡 TODO
   │  ├─ add_subdirectory(games/arena)        🟡 TODO
   │  └─ add_subdirectory(games/battlespire)  🟡 TODO
   │
   ├─ xngine/
   │  ├─ CMakeLists.txt (core library)
   │  └─ 12+ header/source files
   │
   └─ games/
      ├─ redguard/
      │  ├─ CMakeLists.txt → game_redguard.dll ✅
      │  └─ 9 files
      │
      ├─ daggerfall/
      │  ├─ CMakeLists.txt → game_daggerfall.dll 🟡
      │  └─ (9 files when created)
      │
      ├─ arena/
      │  ├─ CMakeLists.txt → game_arena.dll 🟡
      │  └─ (9 files when created)
      │
      └─ battlespire/
         ├─ CMakeLists.txt → game_battlespire.dll 🟡
         └─ (9 files when created)
```

**Build Output:**
```
build/Release/
├─ game_redguard.dll     ✅ (Redguard game plugin)
├─ game_daggerfall.dll   🟡 (Daggerfall game plugin - planned)
├─ game_arena.dll        🟡 (Arena game plugin - planned)
└─ game_battlespire.dll  🟡 (Battlespire game plugin - planned)

All copy to: MO2 installation/plugins/
```

## Data Flow: Game Detection

```
User launches MO2
         │
         ↓
MO2 loads all IPluginGame plugins
         │
         ├─→ game_redguard.dll (GameRedguard)
         ├─→ game_daggerfall.dll (GameDaggerfall)
         ├─→ game_arena.dll (GameArena)
         └─→ game_battlespire.dll (GameBattlespire)
         │
         ↓
Each plugin calls detectGame() / identifyGamePath()
         │
         ├─ GameRedguard::identifyGamePath()
         │  ├─ Check: "...The Elder Scrolls Adventures Redguard/DOSBox-0.73"
         │  ├─ Check: "...Redguard/DOSBOX"
         │  └─ Return path or empty string
         │
         ├─ GameDaggerfall::identifyGamePath()
         │  ├─ Check: "...The Elder Scrolls Daggerfall/DOSBox-0.74"
         │  ├─ Check: "...Daggerfall/DOSBox-0.74"
         │  └─ Return path or empty string
         │
         └─ ...similar for Arena, Battlespire...
         │
         ↓
MO2 displays detected games in dropdown
```

## Data Flow: Mod Validation

```
User adds mod to MO2
         │
         ↓
MO2 calls IPluginGame::looksValid()
         │
         ├─→ GameXngine::looksValid()
         │   └─ Calls ModDataChecker
         │
         ↓
ModDataChecker validates mod
         │
         ├─ GameRedguard registers RedguardsModDataChecker
         │  ├─ Check: About.txt (Format 1 indicator)
         │  ├─ Check: *Changes.txt (INI/Map/RTX changes)
         │  ├─ Check: Redguard-specific folders (data, maps, textures)
         │  ├─ Check: Redguard-specific extensions (.rgm, .rtx, .dat, .mif)
         │  └─ Return VALID/INVALID
         │
         ├─ GameDaggerfall would register DaggerfallsModDataChecker
         │  └─ Similar pattern with Daggerfall-specific patterns
         │
         └─ ...similar for Arena, Battlespire...
         │
         ↓
MO2 displays validation result
```

## Game-Specific Implementation Matrix

| Feature | Engine Level | Game-Specific Override |
|---------|--------------|----------------------|
| Game Detection | Stub in GameXngine | ✅ Each game overrides identifyGamePath() |
| Save Mapping | Generic in GameXngine | ✅ Each game overrides savesDirectory() |
| Executables | Stub in GameXngine | ✅ Each game overrides executables() |
| Mod Validation | Generic XngineModDataChecker | ✅ Each game defines custom <game>sModDataChecker |
| Content Types | Generic XngineModDataContent | ⚪ Inherited (can override if needed) |
| Save Info Display | Generic XngineSaveGameInfo | ⚪ Inherited (can override if needed) |
| Local Saves | Generic XngineLocalSavegames | ⚪ Inherited (can override if needed) |

## Dependency Graph

```
User Applications (MO2)
         ↓
    ┌────────────┐
    │ MO2 SDK    │
    │ (MOBase)   │
    └────────────┘
         ↑
    ┌────────────────┐
    │ XnGine Core    │
    │ (xngine/)      │
    └────────────────┘
    ↑    ↑    ↑    ↑
    │    │    │    └─→ XngineUnmanagedMods
    │    │    │
    │    │    └─→ XngineScriptExtender
    │    │
    │    └─→ XngineLocalSavegames
    │
    └─→ XngineModDataChecker
         XngineModDataContent
         XngineSaveGameInfo
         ... (12 total core classes)
    
    ↑    ↑    ↑    ↑
    │    │    │    └─→ Game Modules
    │    │    │         ├─ GameRedguard
    │    │    │         ├─ GameDaggerfall
    │    │    │         ├─ GameArena
    │    │    │         └─ GameBattlespire
    │    │    │
    │    │    └─→ <Game>sModDataChecker
    │    │         <Game>sModDataContent
    │    │         <Game>sSaveGame
    │    │
    └────└─────┴── NO BACKWARD DEPENDENCIES
              (Games don't depend on each other)
              (Engine doesn't depend on games)
```

## File System Organization

```
modorganizer-game_redguard/
│
├─ Documentation (Markdown)
│  ├─ DOCUMENTATION_INDEX.md          ← START HERE
│  ├─ PROJECT_STATUS_DASHBOARD.md
│  ├─ PHASE2_COMPLETION_SUMMARY.md
│  ├─ XNGINE_GAMES_REFERENCE.md
│  ├─ QUICK_REFERENCE_GAME_PLUGINS.md
│  ├─ XNGINE_GAME_MODULE_CHECKLIST.md
│  ├─ PHASE2B_REDGUARD_PER_GAME_MODULE.md
│  ├─ SESSION_SUMMARY_REDGUARD_MODULE.md
│  ├─ README.md (original)
│  ├─ MODFORMAT.md
│  ├─ REFACTOR_PLAN_XnGine.md
│  └─ XN_GINE_ENGINE_VS_GAME_CLASSIFICATION.md
│
├─ src/
│  ├─ CMakeLists.txt                  ← Build config (add game subdir includes)
│  │
│  ├─ xngine/                          ← ENGINE CORE (Generic XnGine)
│  │  ├─ gamexngine.h/cpp
│  │  ├─ xnginemoddatachecker.h/cpp
│  │  ├─ xnginemoddatacontent.h/cpp
│  │  ├─ xnginesavegame.h/cpp
│  │  ├─ xnginesavegameinfo.h/cpp
│  │  ├─ xnginelocalsavegames.h/cpp
│  │  ├─ xnginegameplugins.h/cpp
│  │  ├─ xnginebsainvalidation.h/cpp
│  │  ├─ xnginescriptextender.h/cpp
│  │  ├─ xngineunmanagedmods.h/cpp
│  │  └─ 2+ more files
│  │
│  ├─ games/                          ← GAME MODULES
│  │  │
│  │  ├─ README.md                    (Architecture guide)
│  │  │
│  │  ├─ redguard/                    ✅ COMPLETE
│  │  │  ├─ CMakeLists.txt
│  │  │  ├─ gameredguard.h/cpp
│  │  │  ├─ gameredguard.json
│  │  │  ├─ redguardsmoddatachecker.h
│  │  │  ├─ redguardsmoddatacontent.h/cpp
│  │  │  ├─ redguardsavegame.h/cpp
│  │  │  └─ (9 files total)
│  │  │
│  │  ├─ daggerfall/                  🟡 PLANNED
│  │  │  └─ (empty - same structure as redguard)
│  │  │
│  │  ├─ arena/                       🟡 PLANNED
│  │  │  └─ (empty - same structure as redguard)
│  │  │
│  │  └─ battlespire/                 🟡 PLANNED
│  │     └─ (empty - same structure as redguard)
│  │
│  ├─ gameredguard.h/cpp              (Original implementation - reference)
│  ├─ RGMODFrameworkWrapper.h/cpp     (Advanced mod loading - reference)
│  ├─ RedguardDataChecker.h/cpp       (Original validator - reference)
│  └─ ... (other original files)
│
├─ build/                            ← CMake build directory
│  ├─ CMakeCache.txt
│  ├─ build.ninja
│  └─ Release/
│     ├─ game_redguard.dll          ✅ (when built)
│     ├─ game_daggerfall.dll        🟡 (when created)
│     ├─ game_arena.dll             🟡 (when created)
│     └─ game_battlespire.dll       🟡 (when created)
│
├─ bin/                             ← Compiled output
│  └─ game_redguard.dll
│
├─ CMakeLists.txt                   (Main build config)
├─ build_ms.bat                     (Build script for Windows)
├─ Deploy.ps1                       (Deploy script)
└─ ... (other config files)
```

## Development Workflow

```
1. PLAN
   └─→ Define game-specific detection, saves, executables
       (See XNGINE_GAME_MODULE_CHECKLIST.md)

2. IMPLEMENT
   └─→ Copy Redguard module template
       └─→ Customize for new game (10-20 files of editing)
       └─→ Test compilation

3. BUILD
   └─→ CMake configure
       └─→ CMake build
       └─→ game_<game>.dll created

4. TEST
   └─→ Load in MO2
       └─→ Verify game detection
       └─→ Verify mod validation
       └─→ Verify save mapping
       └─→ Verify executables list

5. DOCUMENT
   └─→ Update XNGINE_GAMES_REFERENCE.md
       └─→ Update PROJECT_STATUS_DASHBOARD.md
       └─→ Mark as complete in XNGINE_GAME_MODULE_CHECKLIST.md
```

## Key Design Principles

1. **Separation of Concerns**
   - Engine logic (xngine/) never knows about specific games
   - Game logic (games/) only knows about its own game

2. **Inheritance Over Duplication**
   - Common functionality in GameXngine base class
   - Games inherit and override only what's unique

3. **Modularity**
   - Each game compiles to separate DLL
   - Independent testing and deployment

4. **Consistency**
   - Same file naming convention for all games
   - Same class naming pattern (Game<Game>, <game>sModDataChecker)
   - Same directory structure

5. **Extensibility**
   - Easy to add new games (just follow template)
   - No modification to existing games needed

---

This architecture ensures:
- ✅ Clean code organization
- ✅ Easy to maintain and extend
- ✅ Reusable across all 10 XnGine games
- ✅ Parallel development possible
- ✅ Independent testing feasible
