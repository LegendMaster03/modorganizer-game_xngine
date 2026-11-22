# CMAP

## Status
Documented optional.

## Purpose
Inline palette for current image.

## Layout
- Subrecord tag: `CMAP`
- Payload size: `768`
- Content: `256` RGB triplets (`u8,u8,u8`).

## Notes
- When absent, external `.COL` palette path is used.
- Parsers commonly normalize to 256 entries and zero-fill short palette sources.
