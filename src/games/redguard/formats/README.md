# Redguard Format Docs Index

Last updated: 2026-03-11

## Purpose
Canonical local documentation set for Redguard file formats.
This index separates format specs, deep technical appendices, and corpus/research artifacts.

## Documentation Tiers
- Tier 1: format specs (`<FORMAT>.md`) used as first-stop references.
- Tier 2: deep appendices (`*_FULL.md`, behavior/enums) with exhaustive detail.
- Tier 3: corpus/runtime research artifacts (coverage scans, file indexes, parser notes).

## Tier 1: Format Specs
- [SAV/TSG](./SAV/SAV_TSG.md)
- [INI Files](./INI/INI.md)
- [RGM](./RGM/RGM.md)
- [ROB](./ROB/ROB.md)
- [3D / 3DC](./THREED_3DC.md)
- [FNT](./GXA/FNT.md)
- [SFX (MAIN.SFX)](./SFX.md)
- [RTX](./RTX.md)
- [TEXBSI](./TEXBSI/TEXBSI.md)
- [COL](./COL.md)
- [WLD](./WLD.md)
- [PVO](./PVO/PVO.md)
- [GXA](./GXA/GXA.md)
- [LBM](./LBM.md)
- [SWG / SWX](./SWG_SWX.md)
- [SOUP386.DEF](./SOUP386_DEF.md)

## Tier 2: Deep Appendices
- [RGM Enumerations](./RGM/RGM_ENUMS.md)
- [RGM Enumeration Notes](./RGM/RGM_ENUMS_NOTES.md)
- [RGM Script Bytecode (RASC)](./RGM/RGM_SCRIPT_BYTECODE.md)
- [RGM Task/Function Runtime](./RGM/RGM_TASK_FUNCTION_RUNTIME.md)
- [FNT Field Reference](./FNT_FIELDS_FULL.md)
- [SFX Effect Catalog](./SFX_EFFECTS_FULL.md)
- [TEXBSI Field Reference](./TEXBSI/TEXBSI_FIELDS_FULL.md)

## Tier 3: Corpus/Research Artifacts
- [Retail Fileset Coverage](./RETAIL_FILESET_COVERAGE.md)
- [SAV/TSG Block Docs](./SAV/README.md)
- [TSG Files Index (Summary)](./TSG_FILES_INDEX.md)
- `TSG_FILES_INDEX.csv`
- [SAV Chunk Profile (2026-03-11)](./SAV/SAV_TAG_PROFILE_2026-03-11.md)
- [Sound Driver Stack (Miles)](./SOUND_MILES_STACK.md)

## Cross-Cutting
- [Redguard Shared Notes](./REDGUARD_SHARED_NOTES.md)
- [Source References](./SOURCES.md)

## Shared Core Code Anchors
- `src/xngine/xngine3dformat.*`
- `src/xngine/xnginepaletteformat.*`
- `src/xngine/xnginewldformat.*`

## Retired Research-Log Names

The former RGM compatibility and external merge-log files are not canonical. Use `RGM/RGM.md`, `RGM/blocks/*.md`, `SOURCES.md`, and the relevant block docs instead. The former PVO placeholder block pages are also omitted; `PVO/PVO.md` is the container specification.
