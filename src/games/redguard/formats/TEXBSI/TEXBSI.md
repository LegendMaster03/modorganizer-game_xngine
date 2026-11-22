# TEXBSI (Redguard Texture Container)

Last updated: 2026-03-11

## Scope
TEXBSI files in `fxart` store indexed texture data used by 3D model/render pipelines.

## Workspace Implementation Status
- No dedicated Redguard C++ `TEXBSI` reader/writer exists in `src/games/redguard`.
- Current workspace support is documentation-first: the workspace documents the on-disk layout and corpus observations, but no in-repo TEXBSI parser currently consumes the container directly.
- The file remains a semantic reference for future parser work and for interpreting ROB/3D-related texture payloads.

## Retail GOG Validation (Install Corpus)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/fxart`

Observed:
- `415` shards named `TEXBSI.###`.
- Sparse index range `000..500` (non-contiguous).
- `TEXBSI.412` contains `64` images (`E12000..E12063`) in local parse checks.

## Container / File Layout
- Sequence of image structures:
  1. `ImageName` (`u8[9]`)
  2. `ImageSize` (`u32` LE, excludes the 13-byte image header)
  3. image subrecords
- End-of-file image sentinel: all-zero image name.

Subrecord framing:
- `SubrecordName` (`char[4]`)
- `SubrecordSize` (`be u32`, excludes 8-byte subrecord header)

## Section / Block Order
Canonical subrecord order per image:
1. `BSIF` or `IFHD` (mutually exclusive)
2. `BHDR`
3. optional `CMAP`
4. `DATA`
5. `END `

## Endianness
- Default integer/short fields are little-endian unless noted.
- `ImageSize` is little-endian.
- Subrecord size fields are big-endian.

## Blocks
Canonical per-block details are in:
- `blocks/IMAGE_HEADER.md`
- `blocks/BSIF.md`
- `blocks/IFHD.md`
- `blocks/BHDR.md`
- `blocks/CMAP.md`
- `blocks/DATA.md`
- `blocks/END.md`

## Cross-Block Relationships
- `BHDR.FrameCount` controls whether `DATA` is decoded as a flat indexed image (`1`) or animated row-offset table (`>1`).
- Missing `CMAP` falls back to external palette resolution (`.COL` path).

## Open Questions / RE Backlog
1. Resolve semantic labels for BHDR unknown fields.
2. Confirm IFHD-vs-BSIF behavior-level differences beyond structure.
3. Verify animated DATA offset invariants across broader texture corpus.

## Related Docs
- `../COL.md`
- `../ROB.md`
- `../RGM/RGM.md`
- `../TEXBSI_FIELDS_FULL.md`

## Consolidated Source Reference

Consolidated external provenance and caveats are maintained in `../SOURCES.md`.

