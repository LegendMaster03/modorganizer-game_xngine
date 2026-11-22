# THMB - Thumbnail preview block

## Status
Documented (high confidence).

## Purpose
Contains the in-save thumbnail used by the save viewer. This is preview metadata only.

## Canonical Location
- Common observed offset in `SAVEGAME.SAV`: `0x118`
- Parsing should still search by chunk signature, not trust a fixed absolute offset blindly.

## Binary Layout
- Tag[4]: ASCII `THMB`
- Length[4]: big-endian `u32`
- Payload[length]

## Rendering-mode variants

### Glide rendering
- Observed payload length: `0x00018000` (`98,304`)
- Pixel layout: `256 x 192 x 2`
- Encoding: little-endian `RGB565`

Typical bytes:
- `54 48 4D 42 00 01 80 00`

### Software rendering
- Observed payload length: `0x0000C000` (`49,152`)
- Pixel layout: `256 x 192 x 1`
- Encoding: 8-bit indexed pixels
- Palette source: active map palette from `WORLD.INI` -> `world_palette[n]` -> `.COL`

Typical bytes:
- `54 48 4D 42 00 00 C0 00`

## Current parser behavior
- Glide rendering:
  - decodes `0x18000` payloads as `256x192` `RGB565`
- Software rendering:
  - decodes `0x0C000` payloads as `256x192` indexed pixels when a unique active map basename is available and its palette can be loaded
- If neither canonical path succeeds, the parser keeps the old defensive planar fallback

## Validation rules
- `payloadOffset + payloadLength <= fileSize`
- Thumbnail decode failure is non-fatal

## Notes
- The save semantics themselves do not appear renderer-dependent here; only the image payload encoding changes between glide rendering and software rendering.
- Blank thumbnails in software-renderer saves were caused by the old glide-only decoder path.

## Implementation references
- [Redguard save implementation](../../../../redguardsavegame.cpp)
- [Preserved detailed save specification](../SAV_TSG_FULL.md)

## Cross-references
- [SCR3.md](SCR3.md)
- [SCRN.md](SCRN.md)
