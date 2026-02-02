# XnGine Engine vs Game Classification (Install Evidence)

Date: 2026-02-02

Scope
- This document summarizes what appears to be engine-level vs game-specific based on install folder evidence and current source layout.
- Evidence is derived from directory scans of the included Steam and GOG installs in this workspace.

Summary (engine-level vs game-specific)

Engine-level (shared XnGine patterns)
- DOSBox wrapper presence for most titles.
  - Steam layout typically uses DOSBox-0.73/ (or similar version) and a game subfolder inside the install root.
  - GOG layout typically uses DOSBOX/ and game assets at root or in a game-named subfolder.
- Common DOSBox config naming patterns per game (dosbox_<game>.conf and dosbox_<game>_single.conf) for GOG builds.
- Save structure pattern: save data stored in game folders, with per-game naming rules for save directories and .SAV files.
- Root-level GOG metadata markers for store detection (goggame-*.info, goggame-galaxyFileList.ini, goglog.ini).

Game-specific (title-specific rules)
- Redguard:
  - Game data is under Redguard/ in Steam install.
  - Save folder naming: SAVEGAME.xxx (example: SAVEGAME.001) with SAVEGAME.SAV inside each folder.
  - Required config and data files include ENGLISH.RTX, COMBAT.INI, MENU.INI, WORLD.INI, etc.
- Daggerfall:
  - GOG install has save folders SAVE0–SAVE5 at root.
  - Game data includes ARENA2/ directory and executables like DAGGER.EXE, FALL.EXE.
- Arena:
  - GOG install has game data directly at root with many .DAT/.MIF and GLOBAL.BSA.
  - DOSBOX/ and dosbox_arena*.conf files present in GOG install.

Evidence from installs (paths)

Redguard (Steam)
- Root: [f:/SteamLibrary/steamapps/common/The Elder Scrolls Adventures Redguard](f:/SteamLibrary/steamapps/common/The%20Elder%20Scrolls%20Adventures%20Redguard)
- DOSBox wrapper: DOSBox-0.73/
- Game data: Redguard/ with core files (REDGUARD.EXE, ENGLISH.RTX, INI files)
- Save root: Redguard/SAVEGAME/ with SAVEGAME.001 folder

Redguard (GOG)
- Root: [c:/Program Files (x86)/GOG Galaxy/Games/Redguard](c:/Program%20Files%20(x86)/GOG%20Galaxy/Games/Redguard)
- DOSBox wrapper: DOSBOX/
- GOG metadata: goggame-1435829617.info, goggame-galaxyFileList.ini, goglog.ini
- Game data: Redguard/ with REDGUARD.EXE, RGFX.EXE, ENGLISH.RTX, INI files
- Save root: SAVE/ exists, but appears empty in this snapshot

Daggerfall (Steam)
- Root: [f:/SteamLibrary/steamapps/common/The Elder Scrolls Daggerfall](f:/SteamLibrary/steamapps/common/The%20Elder%20Scrolls%20Daggerfall)
- Game data: DF/DAGGER/ with DAGGER.EXE, ARENA2/, and save folders SAVE0–SAVE5

Daggerfall (GOG)
- Root: [c:/Program Files (x86)/GOG Galaxy/Games/Daggerfall](c:/Program%20Files%20(x86)/GOG%20Galaxy/Games/Daggerfall)
- DOSBox wrapper: DOSBOX/
- GOG metadata: goggame-1435829353.info, goggame-galaxyFileList.ini, goglog.ini
- Save root: SAVE0–SAVE5 at root (SAVE0 empty in this snapshot)

Arena (Steam)
- Root: [f:/SteamLibrary/steamapps/common/The Elder Scrolls Arena](f:/SteamLibrary/steamapps/common/The%20Elder%20Scrolls%20Arena)
- DOSBox wrapper: DOSBox-0.74/
- Game data: ARENA/ subfolder

Arena (GOG)
- Root: [c:/Program Files (x86)/GOG Galaxy/Games/Arena](c:/Program%20Files%20(x86)/GOG%20Galaxy/Games/Arena)
- DOSBox wrapper: DOSBOX/
- GOG metadata: goggame-1435828982.info, goggame-galaxyFileList.ini, goglog.ini
- Game data: large set of game files at root (GLOBAL.BSA, ARENA.BAT, etc.)

Implications for refactor
- Detection should prioritize engine-level layout heuristics (DOSBox wrapper + config + metadata) and then apply game-specific file fingerprints (e.g., REDGUARD.EXE + ENGLISH.RTX for Redguard, DAGGER.EXE + ARENA2/ for Daggerfall, GLOBAL.BSA for Arena).
- Save handling must be per-game and avoid VFS mapping for save roots to prevent overwrite pollution.

Open items / TODO
- Confirm Battlespire layout (not present in installs here).
- Confirm Daggerfall Steam save location in DF/DAGGER/SAVE* (evidence shows SAVE0–SAVE5 present).
- Confirm Redguard GOG save location (SAVE/ exists but empty in snapshot).
- Formalize a per-game save schema table in games/<game> modules.
