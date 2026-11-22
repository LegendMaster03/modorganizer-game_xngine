# MPMK

## Status
Documented, partially modeled.

## Purpose
Map markers.

## Record Model
- Header contains `num_items`.
- Marker record size: `12` bytes each.
- Followed by `num_items` footer flag bytes (`enabled/disabled` state).

## Parser Notes
- Unity parser explicitly reads records first, then trailing flag bytes.
- External notes indicate sampled corpora often show all-enabled footer flags.
- Footer bytes are index-aligned to marker records and control per-marker enable/disable behavior.

## Field Snapshot
- Marker record (`12` bytes):
	- `PosX (s24 in 4-byte slot)`
	- `PosY (s24 in 4-byte slot)`
	- `PosZ (s24 in 4-byte slot)`
- Footer:
	- `u8` flag byte per marker index (enabled/disabled)

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
