# BPAL - Bitmap PALette

## Status
Documented

## Purpose
- 256-entry RGB palette used by `GXA` image data.

## Layout
- Chunk framing:
  - `Tag` = `BPAL`
  - `Length` = `be u32`
  - `PayloadLength` = `768`
- Payload:
  - `256` RGB triplets (`u8 r, u8 g, u8 b`)

## Notes
- Source components are usually in 6-bit range (`0..63`) and scale cleanly to 8-bit for display.
- Palette index `0` is treated as transparent in current Redguard image decode paths.

## Writer Notes
- Preserve the raw 768-byte palette when round-tripping.
