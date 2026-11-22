# BMHD - BitMap HeaDer
## Status
Documented

## Purpose
- Per-file image-container header for Redguard `GXA`.
- Carries a short title string and the authoritative image count used to iterate `BBMP`.

## Layout
- Chunk framing:
  - `Tag` = `BMHD`
  - `Length` = `be u32`
  - `PayloadLength` = `34`
- Payload:
  - `0x00` `char[22]` `Title`
  - `0x16` `u8[10]` `Unknown`
  - `0x20` `u16 le` `ImageCount`

## Notes
- `Title` is null-terminated or null-padded ASCII in observed files.
- Example value from `GUI.GXA`: `GXlib image conversion`.
- `ImageCount` is the authoritative image count for the `BBMP` payload, including encoded-image variants.

## Writer Notes
- Preserve the 10 unknown bytes exactly.
- Write `ImageCount` as little-endian.
