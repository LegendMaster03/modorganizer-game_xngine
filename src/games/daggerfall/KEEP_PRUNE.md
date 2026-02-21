# Daggerfall Keep/Prune Classification

## Goal
Classify modules by **project use-case applicability**, not only by current call sites.

This plugin is a mod loader first. Data tooling that supports validation/authoring is applicable, but should not bloat or risk core runtime behavior.

## Use-Case Buckets
1. `Core Runtime`
Required for normal plugin operation: game detection, save support, archive/map-backed metadata.

2. `Applicable Toolkit`
Not required at startup/runtime flow today, but directly useful for the project scope:
advanced validation, mod authoring workflows, data inspection, patch planning.

3. `Optional Unsafe`
Potentially high-risk behavior (binary EXE patching). Keep out of default runtime build.

## Use-Case Matrix
1. `UC1 Runtime Mod Loading`
Discover game, map files, save access, archive-backed resolution.
2. `UC2 Save/World Diagnostics`
Inspect save metadata and world/block references for troubleshooting.
3. `UC3 Quest/Text Authoring`
Parse and validate QRC/QBN, text variables, and resource records.
4. `UC4 Rule/Code Authoring`
BIO/MAGIC/SPELLS/FLATS reference workflows for content authors.
5. `UC5 Asset Inspection`
Image, palette, and ARCH3D parsing for mod previews and validation.
6. `UC6 Legacy Binary Patching`
Explicit user-driven EXE byte patch workflows.

## Classification

### Core Runtime (keep in default build)
- `gamedaggerfall.*`
- `daggerfallsavegame.*`
- `daggerfallmapsbsa.*`
- `daggerfallblocksbsa.*`
- `daggerfallcommon.*`
- `daggerfallformatutils.*`

### Applicable Toolkit (keep, but optional build group)
- `daggerfallpak.*`
- `daggerfallclimatepak.*`
- `daggerfallpoliticpak.*`
- `daggerfallwoodswld.*`
- `daggerfallworldlayers.*`
- `daggerfalltextrsc.*`
- `daggerfalltextrecord.*`
- `daggerfalltextvariables.*`
- `daggerfalltextrscindices.*`
- `daggerfallqbnpseudo.*`
- `daggerfallqbn.*`
- `daggerfallmagicdef.*`
- `daggerfallspellsstd.*`
- `daggerfallbooktxt.*`
- `daggerfallbiotxt.*`
- `daggerfallbiocodes.*`
- `daggerfallflatscfg.*`
- `daggerfallimageformats.*`
- `daggerfallarch3dbsa.*`

Toolkit group mapping:
- `TOOLKIT_TEXT`: `daggerfalltextrsc.*`, `daggerfalltextrecord.*`, `daggerfalltextvariables.*`, `daggerfalltextrscindices.*`, `daggerfallbooktxt.*`
- `TOOLKIT_QUEST`: `daggerfallqbnpseudo.*`, `daggerfallqbn.*`
- `TOOLKIT_WORLD`: `daggerfallpak.*`, `daggerfallclimatepak.*`, `daggerfallpoliticpak.*`, `daggerfallwoodswld.*`, `daggerfallworldlayers.*`
- `TOOLKIT_AUTHORING`: `daggerfallmagicdef.*`, `daggerfallflatscfg.*`, `daggerfallspellsstd.*`, `daggerfallbiotxt.*`, `daggerfallbiocodes.*`
- `TOOLKIT_ASSETS`: `daggerfallimageformats.*`, `daggerfallarch3dbsa.*`

### Optional Unsafe (explicit opt-in only)
- `daggerfallfallexehacks.*`
- `daggerfallfallexeitems.*`

Policy for EXE patch data:
- Do not hardcode patch tables in C++.
- Load patch definitions from user-provided JSON at runtime/tool time.
- Example schema file: `src/games/daggerfall/fall_exe_patches.example.json`.

## CMake Policy
- `XNGINE_DAGGERFALL_ENABLE_TOOLKIT` (default `ON`)
- `XNGINE_DAGGERFALL_ENABLE_EXE_PATCHING` (default `OFF`)
- `XNGINE_DAGGERFALL_TOOLKIT_TEXT` (default `ON`)
- `XNGINE_DAGGERFALL_TOOLKIT_QUEST` (default `ON`)
- `XNGINE_DAGGERFALL_TOOLKIT_WORLD` (default `ON`)
- `XNGINE_DAGGERFALL_TOOLKIT_AUTHORING` (default `ON`)
- `XNGINE_DAGGERFALL_TOOLKIT_ASSETS` (default `ON`)

This preserves long-term capability while keeping default behavior professional and loader-focused.

## Keep/Prune Guidance
1. Keep `Core Runtime` always enabled.
2. Keep toolkit modules in-tree, but gate by feature options.
3. Avoid deleting toolkit code until replacement workflows exist.
4. Keep EXE patching disabled by default and explicitly user-invoked only.
