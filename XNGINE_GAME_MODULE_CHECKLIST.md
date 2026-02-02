# XnGine Game Module Creation Checklist

Track progress of creating game-specific modules following the Redguard template.

## Daggerfall (Priority 1)

**Status:** 🟡 Not Started

### Pre-Implementation
- [ ] Identify Nexus MO Organizer ID (Game ID is 232)
- [ ] Confirm Steam/GOG installation paths
- [ ] Verify save directory structure (SAVE0-SAVE5)
- [ ] Confirm DOSBox versions (0.74 for both Steam/GOG)
- [ ] List game executables and launchers

### Implementation
- [ ] Create `src/games/daggerfall/` directory
- [ ] Create `gamedaggerfall.h` (GameDaggerfall : GameXngine)
- [ ] Create `gamedaggerfall.cpp` with:
  - [ ] `identifyGamePath()` - Search for DAGGER.EXE or DF/DAGGER/
  - [ ] `savesDirectory()` - Return SAVE0-SAVE5 location
  - [ ] `executables()` - DOSBox launcher + standalone exe
  - [ ] Nexus IDs: Game=232, MO=?
  - [ ] Steam App ID: 275170
  - [ ] Other IPluginGame methods
- [ ] Create `daggerfallsmoddatachecker.h` with Daggerfall-specific patterns
  - [ ] Folder names: data, textures, sounds, maps, etc.
  - [ ] Extensions: dat, mid, img, pal, fnt, bsa, txt
- [ ] Create `daggerfallsmoddatacontent.h/cpp`
- [ ] Create `daggerfallsavegame.h/cpp`
- [ ] Create `gamedaggerfall.json` metadata
- [ ] Create `CMakeLists.txt` for build

### Testing
- [ ] Build succeeds without errors
- [ ] DLL created in build/Release/
- [ ] MO2 detects Daggerfall installation
- [ ] Executables list appears in launcher
- [ ] Save games listed correctly
- [ ] Mods validated and loaded

### Documentation
- [ ] Update XNGINE_GAMES_REFERENCE.md
- [ ] Add Nexus MO ID when found
- [ ] Document any game-specific quirks

---

## Arena (Priority 2)

**Status:** 🟡 Not Started

### Pre-Implementation
- [ ] Identify Nexus MO Organizer ID (Game ID is 3)
- [ ] Confirm Steam/GOG installation paths
- [ ] Verify save directory structure
- [ ] Confirm DOSBox version (0.74)
- [ ] Locate game executables (ARENA.EXE, etc.)

### Implementation
- [ ] Create `src/games/arena/` directory
- [ ] Create `gamearena.h` (GameArena : GameXngine)
- [ ] Create `gamearena.cpp` with:
  - [ ] `identifyGamePath()` - Search for ARENA.EXE or GLOBAL.BSA
  - [ ] `savesDirectory()` - Determine Arena save path
  - [ ] `executables()` - List available launchers
  - [ ] Nexus IDs: Game=3, MO=?
  - [ ] Steam App ID: ? (if on Steam)
  - [ ] Other IPluginGame methods
- [ ] Create `arenasmoddatachecker.h` with Arena-specific patterns
  - [ ] Folder names: based on Arena structure
  - [ ] Extensions: bsa, dat, img, pal, fnt, txt
- [ ] Create `arenasmoddatacontent.h/cpp`
- [ ] Create `arenasavegame.h/cpp`
- [ ] Create `gamearena.json` metadata
- [ ] Create `CMakeLists.txt` for build

### Testing
- [ ] Build succeeds without errors
- [ ] DLL created in build/Release/
- [ ] MO2 detects Arena installation
- [ ] Executables list appears in launcher
- [ ] Save games listed correctly
- [ ] Mods validated and loaded

### Documentation
- [ ] Update XNGINE_GAMES_REFERENCE.md
- [ ] Add Nexus MO ID when found
- [ ] Document any game-specific quirks

---

## Battlespire (Priority 3)

**Status:** 🟡 Not Started

### Pre-Implementation
- [ ] Confirm Nexus IDs: Game=1788, MO=?
- [ ] Confirm GOG-only installation (no Steam)
- [ ] Verify save directory structure
- [ ] Confirm DOSBox version and config files
- [ ] Locate game executables (BATTLESP.EXE, etc.)

### Implementation
- [ ] Create `src/games/battlespire/` directory
- [ ] Create `gamebattlespire.h` (GameBattlespire : GameXngine)
- [ ] Create `gamebattlespire.cpp` with:
  - [ ] `identifyGamePath()` - Search for BATTLESP.EXE or Battlespire configs
  - [ ] `savesDirectory()` - Determine Battlespire save path
  - [ ] `executables()` - List available launchers
  - [ ] Nexus IDs: Game=1788, MO=?
  - [ ] Steam App ID: N/A (GOG only)
  - [ ] Other IPluginGame methods
- [ ] Create `battlespiresmoddatachecker.h` with Battlespire-specific patterns
  - [ ] Folder names: based on Battlespire structure
  - [ ] Extensions: dat, img, pal, fnt, txt
- [ ] Create `battlespiresmoddatacontent.h/cpp`
- [ ] Create `battlespiresavegame.h/cpp`
- [ ] Create `gamebattlespire.json` metadata
- [ ] Create `CMakeLists.txt` for build

### Testing
- [ ] Build succeeds without errors
- [ ] DLL created in build/Release/
- [ ] MO2 detects Battlespire installation (GOG)
- [ ] Executables list appears in launcher
- [ ] Save games listed correctly
- [ ] Mods validated and loaded

### Documentation
- [ ] Update XNGINE_GAMES_REFERENCE.md
- [ ] Add Nexus MO ID when found
- [ ] Document any game-specific quirks

---

## Other Games (Priority 4)

**Status:** 🟠 Research Needed

Games requiring investigation:
- [ ] Shadowcrest Isle
- [ ] The Demon Prince
- [ ] An Elder Scrolls Blades
- [ ] Redguard: Cauldron
- [ ] Daggerfall Chronicles
- [ ] Daggerfall: The Bloodied King

For each game:
- [ ] Research Nexus availability/IDs
- [ ] Determine installation paths
- [ ] Verify XnGine compatibility
- [ ] Check mod community activity
- [ ] Create module if justified

---

## Build System Updates

### Main CMakeLists.txt

Update `src/CMakeLists.txt` to include all game modules:

```cmake
# Game-specific plugins
add_subdirectory(games/redguard)    # ✅ Complete
add_subdirectory(games/daggerfall)  # 🟡 WIP
add_subdirectory(games/arena)       # 🟡 WIP
add_subdirectory(games/battlespire) # 🟡 WIP
```

Status:
- [ ] Redguard included in CMakeLists.txt
- [ ] Daggerfall included when complete
- [ ] Arena included when complete
- [ ] Battlespire included when complete
- [ ] All DLLs compile without errors
- [ ] All DLLs deploy to build/Release/

---

## Integration Checklist

After creating each game module:

- [ ] `game<game>.h` created with proper inheritance from GameXngine
- [ ] `game<game>.cpp` implements all required methods
- [ ] `<game>smoddatachecker.h` defines game-specific file patterns
- [ ] `<game>smoddatacontent.h/cpp` (minimal implementation)
- [ ] `<game>savegame.h/cpp` created
- [ ] `game<game>.json` contains correct metadata
- [ ] `CMakeLists.txt` configures build for game module
- [ ] Game module CMakeLists.txt added to main src/CMakeLists.txt
- [ ] All includes use proper xngine/ paths
- [ ] No circular dependencies
- [ ] Proper namespacing in headers

---

## File Naming Convention Verification

For each game `<game>`:

- [ ] `game<game>.h` - Main game plugin header (e.g., `gamedaggerfall.h`)
- [ ] `game<game>.cpp` - Implementation
- [ ] `game<game>.json` - Plugin metadata
- [ ] `<game>smoddatachecker.h` - Mod validator (e.g., `daggerfallsmoddatachecker.h`)
- [ ] `<game>smoddatacontent.h/cpp` - Content categorizer
- [ ] `<game>savegame.h/cpp` - Save handler
- [ ] `CMakeLists.txt` - Build config
- [ ] Directory structure: `src/games/<game>/`

All files follow lowercase naming convention matching Skyrim pattern from modorganizer-game_bethesda.

---

## Testing Matrix

| Game | Build | Detection | Executables | Saves | Mods | Status |
|------|-------|-----------|-------------|-------|------|--------|
| Redguard | ✅ | ? | ? | ? | ? | Created |
| Daggerfall | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | Planned |
| Arena | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | Planned |
| Battlespire | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | Planned |

Legend: ✅ Pass | ❌ Fail | ? Testing | ⬜ Not Started

---

## Notes & Progress

### Completed Work
- ✅ XnGine core refactoring (Phase 2A)
- ✅ Redguard game module created (Phase 2B-1)
- ✅ Gamebryo pattern adopted and documented

### In Progress
- 🟡 Daggerfall analysis and planning
- 🟡 Arena analysis and planning
- 🟡 Battlespire analysis and planning

### Next Steps
1. Identify remaining Nexus MO IDs
2. Create Daggerfall module
3. Create Arena module
4. Create Battlespire module
5. Test all modules with MO2
6. Compile and deploy all DLLs
7. Create end-to-end integration tests

### Known Issues
- None at present (Redguard module awaiting compilation test)

### Research Gaps
- Exact Nexus MO Organizer IDs for Daggerfall, Arena, Battlespire
- Precise save directory structures for each game
- Mod availability and format for each game
- Compatibility of XnGine core with all games' unique requirements
