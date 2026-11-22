# Redguard Design

Last updated: 2026-03-26

## Purpose
Codify the intended structure for Redguard support in this repository before the main rework proceeds.

This document is the working design target for:
- Redguard plugin structure
- parser ownership
- MO2 runtime patching behavior
- Redguard-specific tools
- save-system scope during the rework

## Design Goals
- Preserve the current MO2/VFS runtime patch model.
- Reuse parsers and document models between mod loading and tools.
- Keep Redguard aligned with the structure used by the other game plugins.
- Allow Redguard-specific deviations where they are justified by the format/tooling workload.
- Avoid coupling UI/tool code to binary parsing logic.

## Core Position
Redguard should follow the same high-level project structure as the other games, but it needs more internal layering than Arena or Battlespire.

The best local precedent is Daggerfall:
- small game-plugin shell,
- dedicated format/parser modules,
- optional toolkit-style modules separated from runtime-critical code.

Redguard is expected to be the largest single game-specific undertaking in this project so far.

## Canonical Runtime Model
The current improvement over the old Java loader remains canonical:
- patched files must not overwrite the game install,
- patched outputs are packaged as a temporary runtime mod,
- the temporary mod is applied through MO2's VFS,
- the temporary mod is removed on application close.

The old Java backup/overwrite model is obsolete as a deployment strategy.

## Intended Folder Structure
Redguard should be organized into three logical layers:

1. `src/games/redguard/`
- MO2 game plugin shell
- game detection
- settings
- executable registration
- save registration
- mod data registration
- runtime patch orchestration

2. Redguard format/services layer
- parsers
- document models
- patch application logic
- write-back logic
- shared Redguard utilities

3. `src/games/redguard/tools/`
- MO2 tool integrations
- developer-facing inspection tools first
- later user-facing editors built on the same format/services layer

## Dependency Rules
The dependency direction must stay strict:

- `gameredguard.*` may use Redguard format/services code
- `tools/*` may use Redguard format/services code
- Redguard format/services code must not depend on tool UI code
- parser and write-back logic must not live inside tool classes

In short:
- runtime and tools share the same Redguard format layer
- tools are front ends, not owners of the logic

## What Belongs in the Game Plugin Shell
`GameRedguard` should stay relatively small and should primarily own:
- MO2 plugin lifecycle integration
- game/install detection
- path resolution
- settings exposure
- feature registration
- runtime patched-mod creation and cleanup
- wiring together runtime patch steps

It should not become the long-term home for detailed format logic.

## What Belongs in Redguard-Specific Format/Service Code
The following are Redguard-domain responsibilities and should live outside the plugin shell:
- `RTX` parsing and writing
- `RGM` parsing and writing
- map/script decompile-recompile logic
- SOUP symbol loading and script support
- save parsing
- Redguard-specific patch sidecar handling
- future `TEXBSI`, `ROB`, and related format support

## Shared `xngine` Core Policy
Only move code into shared `xngine` core when the abstraction is genuinely stable across games.

Current model:
- `WLD` is a valid example of shared-core handling with Redguard-specific adaptation.

Do not force Redguard-specific semantics into shared core too early.

In particular, save parsing should not be generalized early.

## Tools Policy
Redguard will likely be the first game in this repo to justify a dedicated `tools` folder under the game module.

This is an intentional and acceptable deviation from the lighter-weight structures used by the other games.

The rationale:
- Redguard requires tooling for development and verification,
- those tools can exercise the same parser and writer paths used by the mod loader,
- developer tools can later be cleaned up into user-facing editors.

## Tooling Roadmap
Tools should be staged:

1. Developer-facing tools
- parser inspection
- structure viewers
- patch preview/diff helpers
- controlled write-back validation tools

2. Limited editing tools
- likely `RTX` first
- focused workflows that directly validate runtime patch behavior

3. User-facing editors
- only after the underlying format/services layer is stable

## Tooling Priority Guidance
Near-term tooling should favor bounded, high-value targets:
- `RTX` is the best early candidate
- script editing should be deferred until parser/runtime confidence is higher

The goal is to support development and testing first, not to rush full editor UX.

## Java Reference Policy
The old Java standalone mod manager remains useful as:
- a behavior reference,
- a parser/writer reference,
- a sidecar-format reference,
- a workflow reference for future tools.

It should not be copied as architecture.

Specifically:
- keep the behavior,
- do not recreate the backup/overwrite deployment model,
- do not port the Swing UI wholesale,
- use it as an oracle for expected parsing and patch-application behavior.

## Existing Redguard Support Policy
Current Redguard support that already works should be preserved during rework where practical.

This especially includes:
- runtime patch application flow
- existing `RTX` support
- existing map/script patch flow
- existing `INI` patch support
- current `WLD` support via shared core

These should be treated as preservation targets, even if the underlying implementation is later reorganized.

## Save System Policy
The save system is currently the least-complete major area.

Working policy:
- save parsing is required,
- save support should remain provisional during the main Redguard rework,
- current save metadata extraction is acceptable as a temporary best-effort layer,
- save parsing must remain isolated from broader architecture decisions until reverse engineering matures.

Implications:
- do not block the rest of Redguard on full save reverse engineering,
- do not treat current location parsing as authoritative,
- do not generalize Redguard save logic into shared core yet.

## Architectural Principles
- Preserve unknown bytes/fields unless there is strong evidence they are safe to normalize.
- Separate parse, model, patch, and write-back concerns.
- Prefer behavior parity over superficial code similarity with the old Java project.
- Keep runtime-required code distinct from optional tools.
- Build tools on top of reusable services, not embedded parser code.
- Keep Redguard-specific complexity contained within Redguard modules.

## Practical Outcome
Redguard should end up as:
- a normal MO2 game plugin at the top level,
- with a deeper game-specific format layer underneath,
- and a Redguard-specific `tools` layer that uses the same parser stack as the mod loader.

That is the intended design.

## Rework Map

### Plugin Shell / Runtime Integration
These files are part of the MO2-facing game plugin shell and should remain at the top layer:
- `gameredguard.cpp`
- `gameredguard.h`
- `gameredguard.json`
- `redguardsmoddatachecker.h`
- `redguardsmoddatacontent.cpp`
- `redguardsmoddatacontent.h`

Target state:
- keep these files relatively small
- keep heavy format logic out of this layer
- use them to call into Redguard services, not to implement those services inline

Current concern:
- `gameredguard.cpp` is overloaded and contains too much detailed Redguard behavior

### Runtime Patch and Mod-Application Support
These files are central to current working Redguard mod support and should be preserved:
- `redguardsmapchanges.cpp`
- `redguardsmapchanges.h`
- `redguardsrtxdatabase.cpp`
- `redguardsrtxdatabase.h`
- `redguardsmapdatabase.cpp`
- `redguardsmapdatabase.h`
- `redguardsitem.cpp`
- `redguardsitem.h`

Target state:
- keep behavior
- clean up API boundaries
- make these reusable from both runtime patching and future tools

### RGM / Script Format Layer
These files are the main Redguard format/service layer and should be one of the first rework targets:
- `redguardsmapfile.cpp`
- `redguardsmapfile.h`
- `redguardsmapheader.cpp`
- `redguardsmapheader.h`
- `redguardsparsedmapheader.h`
- `redguardsscriptinstruction.cpp`
- `redguardsscriptinstruction.h`
- `redguardsscriptparser.cpp`
- `redguardsscriptparser.h`
- `redguardsscriptreader.cpp`
- `redguardsscriptreader.h`
- `redguardssoupflag.cpp`
- `redguardssoupflag.h`
- `redguardssoupfunction.cpp`
- `redguardssoupfunction.h`

Target state:
- stable Redguard-specific service/document layer
- callable from mod loading and future tools
- preserve unknown/optional data safely

### Shared-Core Adaptation
These files are Redguard-side adapters over valid shared-core abstractions:
- `redguardswld.cpp`
- `redguardswld.h`

### Save System
These files should remain isolated from broader architectural decisions until save RE is more mature:
- `redguardsavegame.cpp`
- `redguardsavegame.h`

### Shared Redguard Utility Layer
These files should remain support code used by the Redguard format/services layer:
- `redguardsutils.cpp`
- `redguardsutils.h`

## Module Split
The intended internal split inside `src/games/redguard/` is:

1. Plugin shell
- MO2 integration
- settings
- executable registration
- save registration
- runtime patch orchestration

2. Patch pipeline
- runtime temp-mod packaging
- patch input resolution
- patch execution against base files
- output file generation for VFS use

3. Format services
- `RTX`
- map/script
- SOUP and items
- shared-core adapters like `WLD`

4. Save services
- isolated and provisional

5. Tools
- a future `tools/` folder for MO2-exposed dev tools first, user-facing tools later

Suggested future physical shape:

```text
src/games/redguard/
  gameredguard.cpp
  gameredguard.h
  gameredguard.json
  redguardsmoddatachecker.h
  redguardsmoddatacontent.cpp
  redguardsmoddatacontent.h

  patch/
    ...

  formats/
    ... existing documentation ...

  services/
    rtx/
      ...
    map/
      ...
    script/
      ...
    soup/
      ...
    save/
      ...
    shared/
      ...

  tools/
    ...
```

## Shared-Core Reuse

### Keep Shared In `xngine`
- raw `3D` / `3DC` payload parsing
- palette primitives like `COL`
- shared binary/container helpers of the `WLD` type

### Keep In Redguard As Adapters/Services
- Redguard interpretation of parsed `3D` / `3DC`
- Redguard-specific asset-linkage logic
- Redguard texture-resolution behavior
- Redguard compatibility policies tied to Redguard corpus/runtime behavior

### Keep Fully Redguard-Only
- `ROB`
- `TEXBSI`
- Redguard wrapper/container behavior around shared payload formats

Practical rule:
- `3D` / `3DC` raw payloads stay shared
- Redguard-specific interpretation sits above them
- `ROB` and `TEXBSI` remain Redguard-owned
- `WLD` is the pattern to follow

## Redguard Extraction Status

The first extraction pass is implemented: shared patch utilities live in `redguardspatchutils.*`, while runtime patch orchestration remains separated across the Redguard patch utilities and runtime modules. RTX and map/script patch support is part of the active implementation; new format work should reuse these service boundaries rather than reintroduce logic into `gameredguard.cpp`.

## First Extraction
The first extraction target should be the runtime patch/application logic currently embedded in `gameredguard.cpp`.

### Current patch-pipeline surface
Namespace helpers:
- `profileSuffix(...)`
- `ensureDir(...)`
- `removeDirRecursive(...)`
- `readGlobalXdeltaPath(...)`
- `writeGlobalXdeltaPath(...)`
- `copyDirectoryContents(...)`
- `resolveBaseFilePath(...)`
- `findSoupPath(...)`
- `findMapsRoot(...)`

INI patch functions:
- `parseIniChanges(...)`
- `applyIniChangesToFile(...)`
- `applyIniChanges(...)`

Other patch functions:
- `applyRtxChanges(...)`
- `applyMapChanges(...)`
- `GameRedguard::applyPatchMods()`

### Recommended first code-moving pass
Move first:
- `ensureDir(...)`
- `removeDirRecursive(...)`
- `copyDirectoryContents(...)`
- `resolveBaseFilePath(...)`
- `parseIniChanges(...)`
- `applyIniChangesToFile(...)`
- `applyIniChanges(...)`

Suggested first destination:
- `redguardspatchutils.h`
- `redguardspatchutils.cpp`

Leave in place for the next passes:
- `findSoupPath(...)`
- `findMapsRoot(...)`
- `applyRtxChanges(...)`
- `applyMapChanges(...)`
- `GameRedguard::applyPatchMods()`

### Why this first step
- lowest-risk extraction
- shrinks `gameredguard.cpp` immediately
- preserves working behavior
- creates the first real patch-service boundary

### Validation criteria
- `gameredguard.cpp` becomes smaller
- `INI Changes.txt` behavior remains unchanged
- temp output paths remain unchanged
- no Redguard behavior moves outside `src/games/redguard/`

### Follow-up steps
Step 2:
- extract `RTX` patch logic

Step 3:
- extract map/script patch logic

## Consolidated Project Notes

- Redguard is retained as a dedicated format and service area because its save, model, texture, script, and patch requirements diverge from the shared XnGine implementations.
- The 1.0 priority is stable known/active workflows, defensive diagnostics, and reversible patch behavior, not maximal archaeology.
- Implemented Redguard paths include RTX parsing/editing, RGM read/write and script decompile/recompile, WLD adaptation, partial SAVE parsing, and the runtime patch/sidecar pipeline.
- RGM, TEXBSI, ROB, PVO, GXA, FNT, SFX, and save-system status is tracked in `formats/README.md` and `formats/DOC_GAPS_BACKLOG.md`.
