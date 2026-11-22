# SAV/TSG Corpus Validation

Last updated: 2026-03-11

## Retail GOG Validation (Installed Save Snapshot)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/SAVEGAME/SAVEGAME.001/SAVEGAME.SAV`

Observed marker offsets in sampled retail save:
- File size: `1,678,028` bytes
- Header bytes at `0x00000000`: `53 56 47 4D 00 00 01 10 31 2E 33 31 ...` (`SVGM`)
- First seen chunk tags include:
  - `SVGM` at `0x00000000`
  - `THMB` at `0x00000118`
  - `SCR3` at `0x00018120`
  - `SVCM` at `0x00192776`
  - `SVMV` at `0x00199A0A`
  - `SVIT` at `0x00199272`
  - `SVMD` at `0x00199A21`

Practical confirmation:
- Retail sample matches key tag expectations used by parser/spec.
- Canonical thumbnail location expectation (`THMB` near `0x118`) is validated.

## Extended Save Corpus Validation (MO2 Profile)
Validated against:
- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard`

Observed slot/file structure:
- Nested slot directories `SAVEGAME.001` through `SAVEGAME.038`.
- Slot directories present: `38`; slots with `SAVEGAME.SAV`: `35`.
- Missing-save slot directories: `SAVEGAME.001`, `SAVEGAME.009`, `SAVEGAME.025`.
- All 38 slots contain `LOGBOOK.TXT`; `TSG` counts vary (`1..26`).

Adjacent-slot comparison (supports "broken then fixed" hypothesis):
- `.001 -> .002`: `.001` has 18 TSG, `.002` has 1; overlap hashes match.
- `.009 -> .010`: `.009` has 26 TSG, `.010` has 15; subset by filename, mixed hash matches.
- `.025 -> .026`: `.025` has 26 TSG, `.026` has 22; subset by filename, mixed hash matches.

Diff/inventory artifacts:
- `../SAVE_PAIR_DIFFS.md`
- `../TSG_FILES_INDEX.csv`
- `../TSG_FILES_INDEX.md`

Current inventory snapshot:
- `750` logged `TSG` entries
- `26` distinct `TSG` names
- `38` slot directories represented
- `37` `SAVEGAME.SAV` files scanned (`784,127..1,690,965` bytes)

Current-corpus variation snapshot:
- Distinct gold (`SVIT+0x36`): `0, 19, 53, 85, 95, 110, 181, 202, 244`
- Distinct inferred location codes (`SVMD` heuristic): `?ask, ?bas, ?isz, ?ith, ?leg, ?who` (plus unmatched/empty)

THMB pattern in current corpus:
- `THMB` at canonical offset `0x118` in all scanned saves.
- Post-tag length bytes uniformly `00 01 80 00` (`BE 0x00018000`, 98,304).
- Thumbnail appears corrupted in current saves while save load remains functional.

Known-tag occurrence validation:
- Each known tag appears exactly once per scanned save.
- Full tag ordering is identical across all 37 scanned saves.
- Bounded-length sanity passed (`offset + 8 + be_length <= fileSize`).

Notable first-seen length variability:
- Stable: `SVGM=272`, `THMB=98,304`, `SCR3=614,400`, `SVGF=1,849`, `SVCM=1,016`, `SMFM=7,168`, `SVG2=1,666`, `SVIT=1,896`, `SVCB=32`, `SVMV=15`.
- Variable: `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVRG`, `SVMD`.

`SVIT` cross-file-family observation:
- All scanned `SAVEGAME.SAV` `SVIT` payloads were `1,896` bytes.
- All scanned `.TSG` `SVIT` payloads were also `1,896` bytes.
- This supports a shared binary layout between `SAVEGAME.SAV` and `.TSG` for `SVIT`,
  even though current evidence still says their semantics are not interchangeable.

## Source
- Extracted from `SAV_TSG_FULL.md`.
