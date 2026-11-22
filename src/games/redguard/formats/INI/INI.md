# Redguard INI Files (Tooling-Oriented)

Last updated: 2026-03-11

## Scope
Canonical INI behavior summary for this project, with per-file pages for retail INIs.

Per-file pages:
- `SYSTEM_INI.md`, `WORLD_INI.md`, `ITEM_INI.md`, `MENU_INI.md`
- `COMBAT_INI.md`, `KEYS_INI.md`, `REGISTRY_INI.md`, `SURFACE_INI.md`
- `FOG_INI.md`, `DIG_INI.md`, `MDI_INI.md`

## Text Encoding
- Java tooling reads/writes INI text as IBM437.
- Writer path applies uppercase-accent normalization fallback (`ÁÉÍÓÚ` -> lowercase-accent variants).

## Text + Structure (Generic Parser Model)
- Section header line: `[SectionName]`.
- Lines before first section are stored under an empty section key (`""`).
- Non-header lines are preserved in section order.
- Key/value parsing supports flexible spacing around `=`.
- Duplicate keys collapse to last-seen value in map-based operations.

## INIChanges Sidecar Format
Tool-specific format (`INIChanges.java`):
- top-level non-indented line: target INI filename
- following indented lines: INI payload for that file
- parser groups by filename and applies via same INI reader

This is not retail game INI syntax.

Writer behavior:
- emits `FileName` line followed by two-space-indented INI payload for that file.

Merge behavior (tool path):
- if a section does not exist: copied in full,
- if section exists: per-key overwrite/insert,
- output re-emitted in insertion order of parsed key map.

Caveat:
- comments/non-key lines may not round-trip unchanged when a section is rewritten through key-map merges.

## Implemented Parsing Coverage (Current Workspace)

### `GameRedguard` plugin (`gameredguard.cpp`)
- Declares core INI set for exposure/apply:
  - `COMBAT.INI`, `ITEM.INI`, `KEYS.INI`, `MENU.INI`, `REGISTRY.INI`, `surface.ini`, `SYSTEM.INI`, `WORLD.INI`
- Applies `.ini_changes.txt` style updates with section/key overwrite semantics.
- Map pipeline requires and reads:
  - `WORLD.INI` (map ID/name mapping)
  - `ITEM.INI` (item name/description IDs)

### `RedguardsMapDatabase` specifics
- `readWorldFile(...)`: currently consumes `world_map[index]=...` lines to build map ID/name links.
- `readItemsFile(...)`: currently consumes sequential `name=...` then `description=...` pairs.
- Other INI keys in those files are currently preserved as external data but not deeply consumed by this parser path.

### Unity parser cross-check
- Typed key schemas documented for:
  - `ITEM.INI`
  - `WORLD.INI`
  - `MENU.INI`
- `io_tst/initst` also documents comment stripping and strict key tokenization behavior.

Detailed typed key inventories:
- `ITEM_INI.md`
- `WORLD_INI.md`
- `MENU_INI.md`

`io_tst/initst` parser details:
- strips semicolon comments by splitting each line at `;`,
- `[general]` is routed through the same page handler used for `[page0]` while still setting global menu flags,
- `test_map_order` is parsed as comma-separated integer list,
- key parse consumes first two tokens from `Split('=')`, so additional unescaped `=` are not preserved,
- unknown indexed keys are logged and ignored (non-fatal).

## External Behavioral Inventory
- External UESP-style notes for `SYSTEM.INI` are tracked as behavior-oriented context with explicit caveats.
- See `../INI_UESP_MERGE_NOTES_2026-03-11.md`.

## Retail Install INI Inventory (Current Scan)
Observed under `Redguard/`:
- core root INIs: `COMBAT.INI`, `ITEM.INI`, `KEYS.INI`, `MENU.INI`, `REGISTRY.INI`, `surface.ini`, `SYSTEM.INI`, `WORLD.INI`
- additional subdir INIs: `fxart/FOG.INI`, `sound/DIG.INI`, `sound/MDI.INI`
- `backup/*` mirrors for core root set also present.

## Sources
- `redguard-mod-manager-main/src/redguard/INIFile.java`
- `redguard-mod-manager-main/src/redguard/INIChanges.java`
- `src/games/redguard/gameredguard.cpp`
- `src/games/redguard/redguardsmapdatabase.cpp`
- `redguard-unity-master/.../RGINIFile.cs`
- `redguard-unity-master/.../RGINIStore.cs`

## External Source Status

The external SYSTEM.INI option inventory is behavior-oriented and partially speculative. Code-derived parsing remains authoritative.
