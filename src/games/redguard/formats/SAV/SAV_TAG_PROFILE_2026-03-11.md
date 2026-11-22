# SAV Chunk Profile (2026-03-11)

Generated from local available SAVEGAME corpus:
- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard`
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/SAVEGAME`

Purpose:
- preserve measured chunk-level invariants used to update `SAV_TSG.md`.

## Corpus Summary
- Total `SAVEGAME.SAV` scanned: `37`
- Input roots:
	- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard`
	- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/SAVEGAME`
- File-size range: `784,127..1,690,965`
- Distinct full tag orders observed: `1`
- Unexpected chunk tags: none
- Out-of-bounds chunk rows: `0`

## Canonical Full Chunk Order (all 37 files)
`SVGM > THMB > SCR3 > SVGF > SMGR > SMOB > SMRP > SMCO > SMSH > SVCM > SMFM > SVRG > SVGV > SVG2 > SVIT > SVCB > SVMV > SVMD`

## THMB Invariant
- Offset: `0x118` (`280`) in all scanned saves.
- Field after tag (`be u32` length): always `0x00018000` (`98,304`).
- Boundedness check: true for all samples.

## Tag Length Ranges (be u32)

| Tag | MinLen | MaxLen | Notes |
|---|---:|---:|---|
| `SVGM` | 272 | 272 | stable |
| `THMB` | 98,304 | 98,304 | stable |
| `SCR3` | 614,400 | 614,400 | stable |
| `SVGF` | 1,849 | 1,849 | stable |
| `SMGR` | 32,812 | 36,164 | variable |
| `SMOB` | 19,980 | 859,464 | highly variable |
| `SMRP` | 0 | 1,988 | sparse/variable |
| `SMCO` | 624 | 50,544 | variable |
| `SMSH` | 198 | 564 | variable |
| `SVCM` | 1,016 | 1,016 | stable |
| `SMFM` | 7,168 | 7,168 | stable |
| `SVRG` | 1,692 | 15,956 | variable |
| `SVGV` | 1,542 | 1,542 | stable |
| `SVG2` | 1,666 | 1,666 | stable |
| `SVIT` | 1,896 | 1,896 | stable |
| `SVCB` | 32 | 32 | stable |
| `SVMV` | 15 | 15 | stable |
| `SVMD` | 9 | 519 | variable |

## Offset Ranges (tag start)

| Tag | MinOffset | MaxOffset |
|---|---:|---:|
| `SVGM` | 0 | 0 |
| `THMB` | 280 | 280 |
| `SCR3` | 98,592 | 98,592 |
| `SVGF` | 713,000 | 713,000 |
| `SMGR` | 714,857 | 714,857 |
| `SMOB` | 747,677 | 751,029 |
| `SMRP` | 767,689 | 1,610,501 |
| `SMCO` | 767,697 | 1,610,651 |
| `SMSH` | 768,537 | 1,661,203 |
| `SVCM` | 768,743 | 1,661,409 |
| `SMFM` | 769,767 | 1,662,433 |
| `SVRG` | 776,943 | 1,669,609 |
| `SVGV` | 778,735 | 1,685,573 |
| `SVG2` | 780,285 | 1,687,123 |
| `SVIT` | 781,959 | 1,688,797 |
| `SVCB` | 783,863 | 1,690,701 |
| `SVMV` | 783,903 | 1,690,741 |
| `SVMD` | 783,926 | 1,690,764 |

## Notes
- This artifact is intentionally corpus-scoped.
- Additional saves from different progression phases may reveal additional variants.
