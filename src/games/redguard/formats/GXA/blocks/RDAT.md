# RDAT - RDAT

## Status
Documented optional block

## Purpose
- Optional trailing metadata block in `FNT`.
- Reported to be runtime-unused export/source metadata.

## Layout
- Chunk framing:
  - `Tag` = `RDAT`
  - `Length` = `be u32`
- Common observed payload size:
  - `173` bytes
- Observed field map for the 173-byte form:
  - `0x00` `char[136]` `SourceFile`
  - `0x88` `u32 le` `Unknown_90`
  - `0x8C` `u32 le` `Unknown_94`
  - `0x90` `u32 le` `Unknown_98`
  - `0x94` `u32 le` `Unknown_9C`
  - `0x98` `u32 le` `Unknown_A0`
  - `0x9C` `u32 le` `Unknown_A4`
  - `0xA0` `u32 le` `Unknown_A8`
  - `0xA4` `u32 le` `Unknown_AC`
  - `0xA8` `u32 le` `Unknown_B0`
  - `0xAC` `u8` `Unknown_B4`

## Notes
- `FNHD.HasRDAT` indicates whether this block should be present.
- Unknown-field semantics remain unresolved, but the field positions are stable enough to parse.

## Writer Notes
- Preserve unknown values exactly.
- If a non-173-byte variant is encountered, treat it as opaque payload unless a fuller map is known.
