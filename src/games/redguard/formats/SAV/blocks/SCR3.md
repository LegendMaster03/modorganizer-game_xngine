# SCR3

## Status
Documented (observed, parser-backed).

## Purpose
Carries the full glide-rendering framebuffer in saves that use glide rendering.

## Binary Layout
- Chunk framing: `Tag[4] + be u32 length + payload`.
- Observed payload length: `0x00096000` (`614,400`)
- Pixel layout: `640 x 480 x 2`
- Encoding: little-endian `RGB565`

## Notes
- Observed in glide-rendering `SAVEGAME.SAV`
- Not observed in the software-rendering sample where `SCRN` is used instead
- Used as a screenshot fallback after `THMB`

## Current parser behavior
- If `THMB` does not decode and `SCR3` is present with length `614400`, the parser decodes it as a `640x480` `RGB565` image

## References
- [Redguard save implementation](../../../../redguardsavegame.cpp)
- [THMB.md](THMB.md)
- [SCRN.md](SCRN.md)
