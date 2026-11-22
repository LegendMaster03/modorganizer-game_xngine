# RAVA

## Status
Documented + partially implemented.

## Purpose
Local variable payload sets per script/object instance.

## Record Model
- Stored as `u32 le[]` values.
- First value in a set is variable-count style metadata.
- Field-reference naming from full map:
	- `Var00`: count/size-style value including itself,
	- `Var01..`: remaining variable payload entries.

## Relationships
- `RAHD.RAVAOffset` is a byte offset (commonly divided by `4` for `u32` indexing).
- Total variable sets are linked via RAHD + MPOB instance model.
- Variable-set slicing can be derived by either:
	- `RAHD.RAVA_Length / RAHD.MPOB_Record`, or
	- `RAHD.RAVA_Length / (RAHD.RAVA_Record * 4)`.
- Set-to-instance link is by index over MPOB entries that use the RAHD header.
- Invariant from corpus notes: first value (`Var00`) includes itself and generally matches `RAHD.RAVA_Record`.

## Writer Notes
- Java writer emits leading 4 zero bytes (`RAVA[0]` sentinel slot) before payload.
