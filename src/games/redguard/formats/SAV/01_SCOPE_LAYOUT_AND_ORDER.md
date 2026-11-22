# SAV/TSG Scope, Layout, and Order

Last updated: 2026-03-11

## Scope
This section documents container framing, slot layout rules, and canonical chunk order currently observed.

## Container / File Layout
- Save slots are directory-based.
- Slot directory regex: `(?i)^SAVEGAME\.(\d+)$`.
- Valid slot requires `SAVEGAME.SAV` inside slot directory.
- Typical structure:
  - `SAVEGAME/SAVEGAME.001/SAVEGAME.SAV`
  - `SAVEGAME/SAVEGAME.001/*.TSG` and other auxiliary files
- Parser tracks auxiliary files in slot folder (all files except `SAVEGAME.SAV`).

Important corpus correction:
- Some slot directories contain many `*.TSG` files and `LOGBOOK.TXT` but no `SAVEGAME.SAV`.
- Current working hypothesis: these are broken/incomplete snapshots due to missing `SAVEGAME.SAV`.
- Adjacent-slot evidence supports this (`.001/.009/.025` vs following SAV-backed slots), but runtime loadability is still unverified.

Top-level `SAVEGAME.SAV` framing used by current parser:
- `Tag[4]` (ASCII marker)
- `Length[4]` (BE `u32` in scanner path)
- `Payload[Length]`

Terminator behavior:
- No single file-level `END ` chunk currently documented for `SAVEGAME.SAV`.

## Section / Block Order
Observed canonical full order in current local corpus (`37` saves):
- `SVGM > THMB > SCR3 > SVGF > SMGR > SMOB > SMRP > SMCO > SMSH > SVCM > SMFM > SVRG > SVGV > SVG2 > SVIT > SVCB > SVMV > SVMD`

Known tag family observed in corpus (single occurrence each in scanned saves):
- `SVGM THMB SCR3 SVGF SMGR SMOB SMRP SMCO SMSH SVCM SMFM SVRG SVGV SVG2 SVIT SVCB SVMV SVMD`

## Common Header (SAVEGAME.SAV pre-chunk)
| Offset | Type | Name | Description |
|---|---|---|---|
| `0x00` | `char[4]` | `Signature` | Expected literal `SVGM` |
| `0x08` | `cstr[<=8]` | `FormatVersion` | Parser-extracted version string |
| `0x14` | `cstr[<=128]` | `SaveTitle` | Parser-extracted save display title |

## Source
- Extracted from `SAV_TSG_FULL.md`.
