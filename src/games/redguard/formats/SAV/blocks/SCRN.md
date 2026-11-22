# SCRN - Software-rendering framebuffer block

## Status
Documented (observed, partially interpreted).

## Purpose
Carries the full software-rendering framebuffer in saves that use software rendering.

## Binary Layout
- Tag[4]: ASCII `SCRN`
- Length[4]: big-endian `u32`
- Payload[length]

## Observed payload
- Observed payload length: `0x0004B000` (`307,200`)
- Pixel layout: `640 x 480 x 1`
- Encoding: 8-bit indexed pixels
- Palette source: active map palette from `WORLD.INI` -> `world_palette[n]` -> `.COL`

## Notes
- Observed in software-rendering `SAVEGAME.SAV`
- Not observed in the glide-rendering sample where `SCR3` is used instead
- This block is used as a screenshot fallback after `THMB`

## Current parser behavior
- If `THMB` does not decode and `SCRN` is present with length `0x4B000`, the parser decodes it as a `640x480` indexed image when the active map palette can be resolved

## Unknowns
- Whether any alternate software-rendering save variants use a different indexed palette source
- Whether `SCRN` ever appears in `.TSG` files

## Implementation references
- [Redguard save implementation](../../../../redguardsavegame.cpp)

## Cross-references
- [THMB.md](THMB.md)
- [SCR3.md](SCR3.md)
