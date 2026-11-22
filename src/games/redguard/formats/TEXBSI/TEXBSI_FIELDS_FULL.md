# TEXBSI Field Reference (Detailed)

Last updated: 2026-03-08

## File-Level Model
- Sequence of image structures.
- Image structure begins with:
  - `ImageName` (`u8[9]`)
  - `ImageSize` (`u32`, LE), excluding these 13 bytes
- All-zero image name marks end of file.
- Java writer appends a terminal trailer after image stream: `u32 0`, `u32 0`, `u8 0`.

## Subrecord Header
Each image subrecord begins with:
- `SubrecordName` (`char[4]`)
- `SubrecordSize` (`be u32`) excluding the 8-byte subrecord header

## Subrecord Order
1. `BSIF` or `IFHD` (mutually exclusive)
2. `BHDR`
3. optional `CMAP`
4. `DATA`
5. `END `

## BSIF
- Size: 0
- Meaning: Bethesda Softworks Image Format marker (reported)
- Usage: used for non-animated images (except noted outlier in TEXBSI.357)

## IFHD
- Size: 44
- Payload: reported fixed bytes (1 followed by 43 zero bytes)
- Constraint: cannot coexist with BSIF

## BHDR (26 bytes)

Reference struct name: `redguard_image_header_t`

| Order | DataType | Values | Notes |
|---|---|---|---|
| 1 | s16 | various | X offset |
| 2 | s16 | various | Y offset |
| 3 | s16 | various | Width |
| 4 | s16 | various | Height |
| 5 | u8 | 0,1 | `Unknown1`; 1 on animated effects like fire/water |
| 6 | u8 | 0,1,9 | `Unknown2` |
| 7 | s16 | 0 | `Zero1` (expected zero) |
| 8 | s16 | 0 | `Zero2` (expected zero) |
| 9 | s16 | 1..16 | `FrameCount` |
| 10 | s16 | 0..500 (mostly 71) | `Unknown3` |
| 11 | s16 | 0 | `Zero3` (expected zero) |
| 12 | s16 | 0 | `Zero4` (expected zero) |
| 13 | u8 | 0,128,163 | `Unknown4`; 128 on fish, 163 on flames |
| 14 | u8 | 0,1,2 (mostly 1) | `Unknown5` |
| 15 | s16 | 0,4 (mostly 0) | `Unknown6` |

## CMAP
- Size: 768
- Content: 256 RGB triplets
- Optional inline palette for current image

## DATA
- Size: variable
- Non-animated (`FrameCount == 1`):
  - `Width * Height` bytes of indexed pixels
- Animated (`FrameCount > 1`):
  - offset table of `Height * FrameCount` little-endian `u32`
  - each row offset points to `Width` bytes
  - row decode rule in Java reference: `rowStart = offsetValue - offsetTableByteLength`
  - Java writer stores offsets as `rowStart + offsetTableByteLength`
  - Java writer may deduplicate identical rows across frames and point multiple table entries to one row payload

## END
- Size: 0
- Marker: `END `

## External Palette Note
- When `CMAP` is missing, texture uses external `.COL` palette files in `fxart`.
- Reference documentation states `WORLD.INI` defines palette selection by location.

## Corpus Notes (external reference)
- Reported 415 TEXBSI files in `fxart`.
- Numeric extensions reported in range `000..500`.
- `TEXBSI.412` reported to contain Cyrus-related textures.
