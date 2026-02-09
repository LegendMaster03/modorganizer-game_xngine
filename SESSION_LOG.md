# Session Log (Battlespire Crash Isolation)

## Goal
Stabilize MO2 loading for Battlespire plugin and isolate crash cause.

## Current Findings
- Crash happens immediately after `GameBattlespire::savesDirectory()` logs its return path.
- Returning game root, documents, or fixed paths did not stop crash (until we discovered MO2 was loading an old DLL).
- Build/deploy mismatch confirmed: MO2 was loading an older DLL; new logs were not appearing.
- CMake task failed due to missing compiler env: `No CMAKE_C_COMPILER` / `No CMAKE_CXX_COMPILER`.

## Build/Deploy State
- VS Build Tools 2022 installed.
- CMake tasks run in a shell without VS dev environment.
- Fix: run CMake with VsDevCmd.bat:
  - `cmd /c "\"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat\" -arch=amd64 && cmake -G \"Visual Studio 17 2022\" -A x64 -B build -DCMAKE_BUILD_TYPE=Release"`
  - `cmd /c "\"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat\" -arch=amd64 && cmake --build build --config Release"`
  - Copy DLLs to MO2 plugins: `Copy-Item build/bin/Release/plugins/*.dll C:/Modding/MO2/plugins/ -Force`

## Key Log Clues (latest)
- Crash always right after: `savesDirectory()` log line.
- Example: `... savesDirectory() using game root: F:/SteamLibrary/...` then crash.
- When MO2 was loading old DLL, logs didn’t match source (fix by deploying correct DLL).

## Code Edits (recent)
- Added build tag log in `GameBattlespire::savesDirectory()` to verify correct DLL load.
- Added `GameBattlespire::detectGame()` override to populate `m_MyGamesPath` when empty.
- `savegameExtension()` / `savegameSEExtension()` temporarily return empty string for isolation.
- `documentsDirectory()` / `savesDirectory()` swapped between variants during isolation.
- `executables()` temporarily disabled (commented) at one stage; re-enabled later.
- `validShortNames()` simplified to `{ "battlespire" }` (original list commented).
- `gameNexusName()` temporarily returned empty string for isolation.

## Current Code Intents (Battlespire)
- `identifyGamePath()` prefers Steam library paths over GOG Program Files.
- `savesDirectory()` currently forced to use documents or game root depending on latest edit.
- Need to confirm which DLL MO2 loads (build tag should appear).

## Next Steps
1. Fix build/deploy pipeline so MO2 loads the new DLL (build tag appears in logs).
2. Re-run MO2 and confirm build tag shows.
3. Re-test crash with known DLL and isolate:
   - If crash persists after `savesDirectory()` log, stub `savegameExtension` and `listSaves` next.
4. Long-term: use `IPluginGame::getModMappings()` / mapper to exclude SAVE* folders from VFS.

## Notes
- Steam install uses full game name; GOG uses short name.
- DOSBox layout differs: Steam has DOSBox in root; Redguard has DOSBox one level deeper.
- Saves are in game root for Battlespire (SAVE0, SAVE1, ...). This conflicts with VFS and should be mapped/excluded later.
