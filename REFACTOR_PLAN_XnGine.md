
# Refactor Plan — modorganizer-game_XnGine

Goal: reorganize this plugin so XnGine-specific, cross-game functionality lives in a reusable `xngine` core, and all Redguard-specific code is split into `src/games/redguard/`, following the structure and best-practices used by `modorganizer-game_bethesda`.

This plan is incremental and test-driven: move a small set of files at a time, update CMake, build and smoke-test, then continue.

---

## High-level mapping (current -> target)

- Core / game entry
  - `src/gameredguard.cpp`, `src/RGMODFrameworkWrapper.*` -> `src/games/redguard/GameRedguard.*` (implements `xngine::IGame`)
- Mod loader / plugin handling
  - `src/ModLoader.cpp` -> `src/xngine/mod/ModLoader.cpp` (core interface + Redguard adapter)
- Mod data checker
  - `src/RedguardDataChecker.cpp` -> `src/games/redguard/RedguardDataChecker.cpp`
- Savegames
  - `src/redguardsavegame.cpp` -> `src/games/redguard/save/RedguardSaveGame.cpp`
- RTX / text database
  - `src/RtxDatabase.cpp` -> `src/xngine/data/RtxDatabase.cpp` (core) + `src/games/redguard/data` if specific parsing differs
- Map/script handling
  - `src/MapChanges.cpp`, `MapFile.cpp`, `MapHeader.cpp`, `ScriptInstruction.cpp`, `MenuFile.cpp` -> `src/games/redguard/data/` (these are game-specific)
- Utilities and logging
  - `src/Utils.cpp`, logging functions -> `src/xngine/common/` (shared helpers)

Files to move (initial):

- `src/Utils.cpp`, `src/Utils.h` -> `src/xngine/common/Utils.*`
- Logging: consolidate `logDebug`, `logType1FileSystem`, `logIniDebug`, `logMapDebug`, `logRtxDebug`, `logCrashDebug` into `src/xngine/common/Logging.*`
- `src/RtxDatabase.cpp`, `RtxDatabase.h` -> `src/xngine/data/RtxDatabase.*`
- Keep `src/gameredguard.cpp` and `src/gameRedguard.cpp` as the game adapter; rename to `src/games/redguard/game_redguard.cpp` and header accordingly.

---

## Phased plan (detailed)

Phase A — Preparation (safe, small changes)

1. Add new directories: `src/xngine/{common,mod,data,interfaces}` and `src/games/redguard/{data,save,ui}`.
2. Create minimal interface headers in `src/xngine/interfaces/`: `IGame.h`, `IModLoader.h`, `IModDataChecker.h`, `ISaveGameHandler.h`. Keep signatures small and stable.
3. Move `Utils.*` and logging into `src/xngine/common/` and update includes in one file (e.g., `RGMODFrameworkWrapper.cpp`) to reference the new headers. Update `CMakeLists.txt` to add a new `xngine_common` object/library.
4. Build and smoke-test (no behavior change expected). Run plugin load to verify logs continue to appear.

Phase B — Interface extraction and adapters

5. Implement thin adapters: keep original function bodies but make them implement `xngine` interfaces. Example: `RGMODFrameworkWrapper` becomes an adapter that implements `IModLoader` and delegates to `xngine` helpers.
6. Move `RtxDatabase` into `src/xngine/data/` and refactor any Redguard-specific parsing into a derived class under `src/games/redguard/` only if needed.
7. Update CMake: add `xngine_core` target (static lib) and a `game_redguard` shared library target that depends on `xngine_core`.

Phase C — Move game-specific components

8. Move all map/script/menu/save handling code into `src/games/redguard/` and adapt includes to reference `xngine` interfaces.
9. Replace direct filesystem or path helpers with `xngine::common::PathHelper` wrappers to standardize behavior across games.
10. Run full build and MO2 plugin smoke tests (apply a small mod, start the game) after each set of moves.

Phase D — Consolidation and multiple-game support

11. Once Redguard is stable in the new layout, add clear README and module registration code similar to `modorganizer-game_bethesda` (per-game modules under `src/games/` with per-game CMake subtargets).
12. Rename repository (or create new repo) `modorganizer-game_XnGine` when ready; update CMake, build presets, `vcpkg.json`, and CI configuration.

---

## CMake & build notes

- Create `xngine_core` as a static library target that exports the interface headers and common code.
- `game_redguard` becomes the shared library plugin (DLL) that links to `xngine_core`.
- Keep the existing `CMakeLists.txt` behavior during migration: incrementally add source files to new targets while leaving original files in place until the adapter compiles.
- Add a `tests/` or `tools/` small test binary (console app) that links to `xngine_core` for unit testing of utility functions.

Example CMake layout (conceptual):

```
add_library(xngine_core STATIC
  src/xngine/common/Utils.cpp
  src/xngine/common/Logging.cpp
  src/xngine/data/RtxDatabase.cpp
)

add_library(game_redguard SHARED
  src/games/redguard/game_redguard.cpp
  src/games/redguard/RedguardDataChecker.cpp
)
target_link_libraries(game_redguard PRIVATE xngine_core ${QT_LIBS} uibase)
```

---

## Testing strategy

- After each change set:
  - `cmake -S . -B build` then `cmake --build build --config Release`
  - Confirm plugin DLL timestamp and copy to MO2 plugins folder if needed.
  - Start MO2, enable the Redguard plugin and check logs (we already have workspace and legacy logs). Verify no regression in applying mods and launching executables.
- Add a small unit test harness for `xngine::common` functions to validate file IO and path helpers.

---

## Risks & mitigations

- Linker/ABI breakages: keep adapters and original symbols where possible and move them in small steps.
- Runtime behavior regressions: test the VFS/injection behavior early (we already have `executableForcedLoads()` and logging hooks).
- Large refactor churn: split work into atomic commits and preserve a working branch at all times.

---

## Timeline & checkpoints (suggested)

- Week 1: Phase A — create layout, move utils/logging, add interfaces, update CMake, build/test.
- Week 2: Phase B — implement adapters, move `RtxDatabase`, add xngine core target, smoke tests.
- Week 3: Phase C — move Redguard-specific data handling, update includes, full integration tests.
- Week 4: Phase D — polish, add per-game module README, prepare rename and CI updates.

---

## Immediate next actions (I will perform on approval)

1. Create the new directory layout and add interface headers in `src/xngine/interfaces/`.
2. Extract `Utils.*` and logging to `src/xngine/common/` and update one file to use the new includes (this proves the layout/CMake change is minimal-risk).
3. Run a clean build and report results.

If you approve, I will start with Immediate Action 1.

---

Authored by: Copilot (pair-programming assistant)

---

## Reference: Known XnGine games

According to Wikipedia, the following games used the XnGine engine. These are candidates for future per-game support under `src/games/` once the `xngine` core is ready:

- The 10th Planet
- Burnout: Championship Drag Racing
- The Elder Scrolls Adventures: Redguard
- The Elder Scrolls II: Daggerfall
- An Elder Scrolls Legend: Battlespire
- NIRA Intense Import Drag Racing
- Skynet (video game)
- The Terminator: Future Shock
- XCar: Experimental Racing

This list is for planning scope and to identify common parsing/format needs across XnGine games.
