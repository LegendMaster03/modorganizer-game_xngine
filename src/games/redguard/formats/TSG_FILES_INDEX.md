# TSG Files Index (Corpus Log)

Last updated: 2026-03-08

## Purpose
This file documents the generated inventory of all `*.TSG` files discovered in current save corpora.
The complete row-level log is stored in:
- `src/games/redguard/formats/TSG_FILES_INDEX.csv`

## Current Inventory Snapshot
- Total logged `TSG` file entries: `750`
- Distinct slot directories represented: `38`
- Distinct `TSG` filenames: `26`
- Source roots currently contributing entries:
  - `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard/SAVEGAME` (`750` entries)

## Distinct TSG Names (26)
- `BELLTOWR.TSG`
- `BRENNANS.TSG`
- `CARTOGR.TSG`
- `CATACOMB.TSG`
- `CAVERNS.TSG`
- `DRINT.TSG`
- `EXTPALAC.TSG`
- `GERRICKS.TSG`
- `HARBTOWR.TSG`
- `HIDEINT.TSG`
- `ISLAND.TSG`
- `JAILINT.TSG`
- `JFFERS.TSG`
- `MGUILD.TSG`
- `NECRISLE.TSG`
- `NECRTOWR.TSG`
- `OBSERVE.TSG`
- `PALACE.TSG`
- `ROLLOS.TSG`
- `SILVER1.TSG`
- `SILVER2.TSG`
- `SMDEN.TSG`
- `START.TSG`
- `TAVERN.TSG`
- `TEMPLE.TSG`
- `VILE.TSG`

## CSV Columns
- `SourceRoot`
- `Slot`
- `FileName`
- `RelativePath`
- `SizeBytes`
- `LastWriteTime`
- `FullPath`

## Notes
- This index is a direct filesystem inventory and is intentionally non-interpretive.
- `TSG` presence does not imply the slot also has `SAVEGAME.SAV`; both modes are observed.
