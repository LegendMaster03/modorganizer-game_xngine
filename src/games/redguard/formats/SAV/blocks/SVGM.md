# SVGM

## Status
Implemented (signature + header metadata usage).

## Purpose
Primary save signature/header chunk.

## Known Structure
- Tag: `SVGM`
- Length: `BE u32`
- Includes parser-used pre-chunk metadata region in current tooling model.

## Known Fields (current parser model)
- Signature at file start: expected literal `SVGM`.
- Header-region fields used by parser:
	- offset `0x08`, `cstr[<=8]`: format/version string.
	- offset `0x14`, `cstr[<=128]`: save title.

## Notes
- C-string extraction truncates at first `\0`, then trims whitespace.
- Parser can continue metadata extraction even when full canonical validity is uncertain.

## Unknowns
- Full internal semantics of `SVGM` payload beyond currently used header fields.

## References
- `../SAV_TSG_FULL.md`
