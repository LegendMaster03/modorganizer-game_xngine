# RALC

## Status
Documented, under-modeled.

## Purpose
Per-script location offsets relative to base object position.

## Record Model
- Fixed record size: `12` bytes.
- Fields: `OffsetX (s32)`, `OffsetY (s32)`, `OffsetZ (s32)`.

## Parser/Runtime Notes
- Unity derives count as `sectionLength / 12`.
- `io_tst` uses `RALCOffset / 12` and `RALCLength / 12` for per-header windows.
- Runtime applies offsets relative to `MPOB` base position.
- Location ID convention from external notes:
	- `LocationID 0` corresponds to base MPOB position,
	- `RALC` records map to `LocationID 1..N` in record order.
- Script behavior linkage notes mention use by movement tasks (`MoveToLocation`, `WanderToLocation`).

## Writer Notes
- Raw payload preserved (copy-window path).

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
