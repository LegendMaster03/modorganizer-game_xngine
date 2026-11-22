# IFHD

## Status
Documented.

## Purpose
Alternate image-format header marker, typically for animated image path.

## Layout
- Subrecord tag: `IFHD`
- Payload size: `44`
- Reported payload pattern: one `1` byte followed by `43` zero bytes.

## Notes
- Mutually exclusive with `BSIF`.
- Used by parser paths as animation decode branch cue in combination with `BHDR.FrameCount`/`DATA` interpretation.
