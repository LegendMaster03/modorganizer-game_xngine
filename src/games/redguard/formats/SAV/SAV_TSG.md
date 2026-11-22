# SAV/TSG File Format (Folder Overview)

Last updated: 2026-03-11

## Scope
Folderized overview for Redguard save-system docs.
This page mirrors the role of `RGM/RGM.md` and routes chunk-level detail to `blocks/*.md`.

## Container / File Layout
- Save slots are directory-based (`SAVEGAME.<n>`).
- Primary save payload is `SAVEGAME.SAV`.
- Auxiliary files include many `*.TSG` plus `LOGBOOK.TXT`.

## SAVEGAME.SAV Chunk Framing
- `Tag[4]` (`ASCII`)
- `Length[4]` (`BE u32`)
- `Payload[Length]`

## Observed Chunk Order (current corpus)
`SVGM > THMB > SCR3 > SVGF > SMGR > SMOB > SMRP > SMCO > SMSH > SVCM > SMFM > SVRG > SVGV > SVG2 > SVIT > SVCB > SVMV > SVMD`

## Block Docs
- See `README.md` for ordered navigation.
- Per-chunk docs live under `blocks/*.md`.

## Split Section Docs (from legacy monolith)
- `01_SCOPE_LAYOUT_AND_ORDER.md`
- `02_CORPUS_VALIDATION.md`
- `03_PARSER_BEHAVIOR.md`
- `04_STATUS_AND_BACKLOG.md`

## Current Parser Coverage Boundary
Implemented chunk handlers in current plugin path:
- `THMB` (thumbnail extraction)
- `SVIT` (gold / health-potion scalar fields)
- `SVMD` (location-code heuristic)

Known but currently opaque in parser:
- `SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVCB`, `SVMV`

## Current Canonical Detailed Spec
- `SAV_TSG_FULL.md` is the preserved detailed cumulative reference.

## Migration Status
- Foundational per-chunk content from `SAV_TSG_FULL.md` has been incorporated into `blocks/*.md`.
- High-volume non-block content has also been split into section docs in this folder.
- Remaining migration is deeper field-level details as new RE evidence lands.

## Related Save Research
- `../TSG_FILES_INDEX.md`
- `../TSG_FILES_INDEX.csv`
- `../SAV_TAG_PROFILE_2026-03-11.md`
- `../SAVE_PAIR_DIFFS.md`

## Slot-State Model

The 38-slot local corpus established that slot state is distributed across `SAVEGAME.SAV`, `*.TSG`, and companion files. `SAVEGAME.SAV` remains significant, but TSG coverage is broad enough that a slot should not be modeled as a single SAV file. The implementation should enumerate the complete slot manifest, report missing/corrupt critical members, and merge metadata using documented precedence rather than false success.
