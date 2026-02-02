# XnGine Games Reference

Complete reference for all 10 XnGine games with Nexus IDs, detection paths, and mod format information.

## All XnGine Games (1996-1999)

| Game | Release | Nexus Game ID | Nexus MO ID | Status |
|------|---------|---------------|-------------|--------|
| The Elder Scrolls Adventures: Redguard | 1998 | 4462 | 6220 | ✅ Module Created |
| The Elder Scrolls Adventures: Daggerfall | 1996 | 232 | N/A | 🟡 WIP |
| The Elder Scrolls: Arena | 1994 | 3 | N/A | 🟡 WIP |
| An Elder Scrolls Legend: Battlespire | 1997 | 1788 | N/A | 🟡 WIP |
| Shadowcrest Isle | ? | ? | N/A | 🟠 Research |
| The Demon Prince | ? | ? | N/A | 🟠 Research |
| An Elder Scrolls Blades | ? | ? | N/A | 🟠 Research |
| Redguard: Cauldron | ? | ? | N/A | 🟠 Research |
| Daggerfall Chronicles | ? | ? | N/A | 🟠 Research |
| Daggerfall: The Bloodied King | ? | ? | N/A | 🟠 Research |

## Game Module Implementations

### Redguard (✅ Complete)

**Location:** `src/games/redguard/`

**Nexus IDs:**
- Game ID: 4462
- MO Organizer ID: 6220

**Game Paths:**
- Steam: `The Elder Scrolls Adventures Redguard/`
- GOG: `Redguard/` (GOG Galaxy/Games/)

**Detection Markers:**
- Steam: `DOSBox-0.73/dosbox.exe`, `Redguard/REDGUARD.EXE`
- GOG: `DOSBOX/dosbox.exe`, `dosbox_redguard.conf`, `dosbox_redguard_single.conf`

**Save Directory:**
- Steam: `Redguard/SAVEGAME/` (contains SAVEGAME.XXX folders)
- GOG: `SAVE/` (contains SAVEGAME.XXX folders)

**Mod Formats:**
- Format 1 (Patch-Based): `About.txt` + `*Changes.txt` files (INIChanges.txt, MapChanges.txt, RTXChanges.txt)
- Format 2 (File-Replacement): `.RGM`, `.RTX`, `.DAT`, `.MIF` files or game data folders

**Executables:**
- Steam DOSBox: `DOSBox-0.73/dosbox.exe` with rg.conf
- GOG DOSBox: `DOSBOX/dosbox.exe` with dosbox_redguard.conf
- Standalone: `Redguard/REDGUARD.EXE`

**Files Created:**
```
gameredguard.h/cpp           # Game plugin class
redguardsmoddatachecker.h    # Mod validation
redguardsmoddatacontent.h/cpp # Content categorization
redguardsavegame.h/cpp       # Save game handler
gameredguard.json            # Plugin metadata
CMakeLists.txt               # Build configuration
```

### Daggerfall (🟡 In Progress)

**Location:** `src/games/daggerfall/`

**Nexus IDs:**
- Game ID: 232
- MO Organizer ID: (to be determined)

**Game Paths:**
- Steam: `The Elder Scrolls Daggerfall/`
- GOG: `Daggerfall/` (GOG Galaxy/Games/)

**Detection Markers:**
- Steam: `DOSBox-0.74/dosbox.exe`, `DF/DAGGER/DAGGER.EXE`
- GOG: `DOSBox-0.74/dosbox.exe`, `dosbox_daggerfall.conf`

**Save Directory:**
- Steam: `DF/DAGGER/SAVE0` - `SAVE5` (6 save slots)
- GOG: `SAVE0` - `SAVE5` at root (6 save slots)

**Mod Formats:**
- Similar to Redguard: Format 1 (patch-based) and Format 2 (file-replacement)
- Specific extensions: `.DAT` (data files), `.MIDI` (music), `.IMG` (images)

**Executables:**
- Steam DOSBox: `DOSBox-0.74/dosbox.exe` with daggerfall config
- GOG DOSBox: `DOSBox-0.74/dosbox.exe` with dosbox_daggerfall.conf
- Standalone: `DAGGER.EXE`

### The Elder Scrolls: Arena (🟡 In Progress)

**Location:** `src/games/arena/`

**Nexus IDs:**
- Game ID: 3
- MO Organizer ID: (to be determined)

**Game Paths:**
- Steam: `The Elder Scrolls Arena/`
- GOG: `Arena/` (GOG Galaxy/Games/)

**Detection Markers:**
- Steam: `DOSBox-0.74/dosbox.exe`, `ARENA/GLOBAL.BSA`, `Arena.exe`
- GOG: `DOSBox-0.74/dosbox.exe`, `GLOBAL.BSA`, arena config files

**Save Directory:**
- Game-specific save location (to be researched)

**Mod Formats:**
- Format 1 & 2 (similar to other XnGine games)
- Specific extensions: `.BSA` (archive), `.DAT`, `.IMG`, `.PAL` (palette)

**Executables:**
- Steam DOSBox: `DOSBox-0.74/dosbox.exe`
- GOG DOSBox: `DOSBox-0.74/dosbox.exe`
- Standalone: `ARENA.EXE` or similar

### An Elder Scrolls Legend: Battlespire (🟡 In Progress)

**Location:** `src/games/battlespire/`

**Nexus IDs:**
- Game ID: 1788
- MO Organizer ID: (to be determined)

**Game Paths:**
- Steam: (Battlespire not on Steam, GOG only)
- GOG: `An Elder Scrolls Legend Battlespire/` (GOG Galaxy/Games/)

**Detection Markers:**
- GOG: `DOSBox/dosbox.exe`, `DOSBOX/dosbox.conf`, `BATTLESP.EXE`

**Save Directory:**
- Game-specific save location (to be researched)

**Mod Formats:**
- Similar to Redguard/Daggerfall but with Battlespire-specific content types

**Executables:**
- GOG DOSBox: `DOSBOX/dosbox.exe` with dosbox.conf
- Standalone: `BATTLESP.EXE`

### Other Games (🟠 Research Needed)

The following 6 games use XnGine but have limited information:

1. **Shadowcrest Isle** - Limited release, DOS-only
2. **The Demon Prince** - Limited release, DOS-only
3. **An Elder Scrolls Blades** - Standalone, mobile, different engine
4. **Redguard: Cauldron** - Expansion or sequel component
5. **Daggerfall Chronicles** - Expansion or companion
6. **Daggerfall: The Bloodied King** - Expansion or companion

These may require:
- Research for correct Nexus IDs (if any)
- Custom game path detection
- Specific save directory mappings
- Game-specific mod format detection

## Implementation Priority

1. **Redguard** ✅ Complete - Serves as reference template
2. **Daggerfall** - High priority (232 on Nexus, most active community)
3. **Arena** - Medium priority (Game ID 3, historically important)
4. **Battlespire** - Medium priority (Game ID 1788, GOG availability)
5. **Others** - Low priority (limited availability, research needed)

## Code Generation Template

Use this template for creating new game modules:

```cpp
// gamex.h
#include "gamexngine.h"

class GameX : public GameXngine {
  Q_PLUGIN_METADATA(IID "org.tannin.GameX" FILE "gamex.json")
  
  // Implement identifyGamePath(), savesDirectory(), executables(), etc.
};

// gamex.cpp
QString GameX::gameName() const { return "Game Name"; }
QString GameX::gameShortName() const { return "x"; }
QString GameX::gameNexusName() const { return "gamename"; }
int GameX::nexusGameID() const { return NEXUS_ID; }
int GameX::nexusModOrganizerID() const { return MO_ID; }
QString GameX::steamAPPId() const { return "APP_ID"; }

QString GameX::identifyGamePath() const {
  QStringList searchPaths = { /* paths */ };
  // Search and detect game
}

QDir GameX::savesDirectory() const {
  // Return save directory
}
```

## Key Configuration Files

For each game, check these installation files:

- **DOSBox configs:** `dosbox.conf`, `dosbox_*.conf`
- **Game executables:** `GAME.EXE`, `DAGGER.EXE`, `ARENA.EXE`, etc.
- **Game data:** `ARENA2/`, `DF/DAGGER/`, `Redguard/`, `data/`, `maps/`
- **Save locations:** `SAVE0-SAVE5`, `SAVEGAME/`, `SAVEGAME.XXX/`

## Future Enhancements

1. **Expanded Save Game Info** - Parse save file headers for character name, level, playtime
2. **Screenshot Viewer** - Display in-game screenshots if saved with mods
3. **Mod Conflict Detection** - Warn about incompatible Format 1/2 combinations
4. **Automatic Mod Installation** - Import From Nexus integration when available
5. **Cloud Save Support** - Sync saves across machines
6. **Multiplayer Support** - If games support networked play

## Research Needed

For games marked as 🟠:

1. Check GOG/Steam for accurate Nexus IDs
2. Investigate save file formats
3. Determine mod framework compatibility
4. Document game-specific file structures
5. Find example mods to understand format requirements

## References

- Nexus Mods: https://www.nexusmods.com/
- Elder Scrolls Lore: https://en.uesp.net/
- XnGine Technical: (consult REFACTOR_PLAN_XnGine.md)
- Game-Specific Docs: (see MODFORMAT.md)
