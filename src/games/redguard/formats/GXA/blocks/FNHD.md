# FNHD - FoNt HeaDer

## Status
Documented

## Purpose
- Header block for `FNT` font files.
- Carries font metrics and glyph-range metadata used to parse `FBMP`.

## Layout
- Chunk framing:
  - `Tag` = `FNHD`
  - `Length` = `be u32`
  - `PayloadLength` = `56`
- Payload:
  - `0x00` `char[32]` `Description`
  - `0x20` `u16 le` `Unknown_24`
  - `0x22` `u16 le` `HasRDAT`
  - `0x24` `u16 le` `Unknown_28`
  - `0x26` `u16 le` `Unknown_2A`
  - `0x28` `u16 le` `Unknown_2C`
  - `0x2A` `u16 le` `MaxWidth`
  - `0x2C` `u16 le` `LineHeight`
  - `0x2E` `u16 le` `CharacterStart`
  - `0x30` `u16 le` `CharacterCount`
  - `0x32` `u16 le` `Unknown_36`
  - `0x34` `u16 le` `Unknown_38`
  - `0x36` `u16 le` `Unknown_3A`

## Notes
- `CharacterStart` and `CharacterCount` define the sequential glyph range stored in `FBMP`.
- `HasRDAT = 1` indicates the optional `RDAT` block is present.

## Writer Notes
- Preserve unknown fields exactly.
- Write all scalar fields little-endian.
