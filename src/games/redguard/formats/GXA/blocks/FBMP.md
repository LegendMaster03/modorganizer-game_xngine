# FBMP - Font BitMaP image

## Status
Documented

## Purpose
- Glyph bitmap payload block for `FNT`.

## Glyph Record Layout
The `FBMP` payload contains `FNHD.CharacterCount` sequential glyph records.

Per-glyph record:
- `0x00` `u16 le` `IsEnabled`
- `0x02` `s16 le` `OffsetLeft`
- `0x04` `s16 le` `OffsetTop`
- `0x06` `u16 le` `Width`
- `0x08` `u16 le` `Height`
- `0x0A` `u8[Width * Height]` `CharacterData`

## Notes
- Glyph order starts at ASCII `FNHD.CharacterStart` and advances sequentially.
- `IsEnabled == 0` disables the glyph; any non-zero value marks it as present.
- `CharacterData` is top-left-origin indexed pixel data.

## Writer Notes
- Iterate exactly `FNHD.CharacterCount` records.
- Write glyph metrics little-endian, followed by raw indexed pixels.
