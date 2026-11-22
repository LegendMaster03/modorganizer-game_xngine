# DATA

## Status
Documented.

## Purpose
Indexed pixel payload for static or animated images.

## Layout
- Subrecord tag: `DATA`
- Payload size: variable

## Decode Modes
### Non-animated (`FrameCount == 1`)
- `Width * Height` indexed bytes.

### Animated (`FrameCount > 1`)
- Offset table: `Height * FrameCount` entries of `u32 le`.
- Row payload blocks: each row is `Width` bytes.
- Base rule used by major parser paths:
  - stored offset references a row position relative to the beginning of `DATA` payload,
  - row start can be computed with offset-table base adjustment (`offset - offsetTableByteLength`) in Java path.
- Offset value `0` refers to start of the offset list base.

## Notes
- Java writer may deduplicate identical rows and point multiple table entries to shared row payload.
- Unity parser uses random-access row seeks and re-aligns stream position after decode.

## Unknowns
- Whether all retail animated files obey one strict offset-base convention without edge-case variants.
