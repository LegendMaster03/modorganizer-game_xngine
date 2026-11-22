# SAV/TSG Parser Behavior and Relationships

Last updated: 2026-03-29

## Cross-Block Relationships
- Chunk extraction helper (`findLastChunkPayload`) uses "last bounded-valid occurrence wins".
- `SVMD` is the primary structural location-identity signal.
- Exact `SAVEGAME.SAV` to `.TSG` `SVMD` equality is treated as an internal identity match, not
  a user-facing name by itself.
- When multiple `.TSG` files share the same `SVMD`, current code narrows them by:
  1. exact `SVCB`
  2. unique `SVCB word5`
  3. unique `SVCB word3`
  4. exact `SVRG`
- In the current local corpus, that stack resolves every observed structurally ambiguous slot
  to one `.TSG` basename.
- `SVIT` must not be interpreted the same way across `SAVEGAME.SAV` and `.TSG`; only the
  `SAVEGAME.SAV` interpretation is currently confirmed.

## Parser Entry / Construction Flow
`RedguardsSaveGame` constructor sequence:
1. `resolveSavePath()`
2. `scanAuxiliaryFiles()`
3. `detectSlotFromFolder()`
4. `parseSaveHeader()`

Notable behavior:
- `m_PCName` forced to `Cyrus`.
- Save display name preference: parsed title -> folder-derived name -> fallback `Redguard Save`.

Chunk scan rules:
- Length interpreted as `BE u32` during chunk scan.
- Each candidate validated with bounds check.
- Truncated/out-of-bounds candidates skipped.
- Last bounded-valid occurrence chosen.

Save-path resolution:
- If input is slot directory, parser prefers `SAVEGAME.SAV`.
- If missing, fallback is first `*.SAV` file by name.
- Auxiliary scan excludes `SAVEGAME.SAV`.

## Parser Coverage Boundary
Hardcoded handled chunk tags in current implementation:
- `THMB`
- `SVIT`
- `SVMD`

Known tags currently opaque in parser:
- `SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVCB`, `SVMV`

## Chunk Table Dumper Utility
- `RedguardsSaveGame::chunkTableDebugText()` outputs tab-delimited chunk tables.
- Columns: `tag`, `tag_offset`, `payload_offset`, `length_be`, `bounds`, `decode_status`.
- `decode_status=parsed` currently limited to `THMB`, `SVIT`, `SVMD`.

## Auxiliary Files (TSG) Behavior
- Parser scans every matching `*.TSG` file in the save folder.
- A legacy token scan still extracts a best-effort area token for developer details only.
- That token is **not** used as the normal user-facing location anymore.
- Structural `.TSG` comparison is now preferred:
  - exact `SVMD`
  - then exact `SVCB`
  - then unique `SVCB word5` match
  - then unique `SVCB word3` match
  - then exact `SVRG`
- Current reverse-engineering on `SVCB` also suggests a likely next field-level discriminator:
  - `SVCB word5` (`+0x14`) appears to behave like a small state flag
  - current runtime code now uses `word5`, then `word3`, but only when that field-level match
    narrows the candidate set to exactly one `.TSG`

## Location Resolution (Current State)
- Loads subtitle entries via `RedguardsRtxDatabase`.
- Structural/internal map identity is currently the strongest proven result:
  1. exact `SVMD`
  2. then exact `SVCB`
  3. then unique `SVCB word5`
  4. then unique `SVCB word3`
  5. then exact `SVRG`
- In the current local corpus, that stack is sufficient to reduce every observed structurally
  ambiguous slot to one `.TSG` basename.
- User-facing display names are **not** currently treated as solved.
- Current implementation still gathers developer evidence from:
  - `SVMD` `?xxxx` labels intersected with matched-map labels
  - parsed-map/script-derived dialogue-comment candidates
  - transition-derived `LoadWorld(<MAP>)` context
- But those evidence sources can still surface non-location subtitles such as `BYE`, `HELP`,
  person/topic names, or other dialogue-only strings on otherwise-correct maps.
- Therefore the normal visible `Location` field should stay empty unless a future derivation path
  is proven structurally.
- `WORLD.INI` is used only as map metadata:
  - to determine canonical map basenames
  - to distinguish outdoor/world maps from interiors
- Raw internal map basenames are not shown directly to the user.

## File Enumeration Behavior
- `allFiles()` returns `SAVEGAME.SAV` and ensures original filepath inclusion.
- Auxiliary files tracked separately as `m_AuxiliaryFiles`.

## Source
- Extracted from `SAV_TSG_FULL.md`.
