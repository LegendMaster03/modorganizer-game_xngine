# MPSL

## Status
Documented, partially modeled.

## Purpose
Light entries.

## Record Model
- Record size: `42` bytes with section-level `RecordCount`.

## Field Highlights
- Contains `WorldID`, signed-24 positions, and light tuple (`Radius`, `Intensity`, `Red`, `Green`, `Blue`).
- Trailing bytes `Unknown_1E..Unknown_29` are opaque and should be preserved.
- External behavior notes:
	- `WorldID == 0` lights are often broad/apply-widely and may be used for shadow/negative-light style effects,
	- non-zero `WorldID` lights are map/world-specific (including ISLAND variant usage in corpus notes).

## Field Snapshot (record size 42)
- `Unknown_00 (u32)`
- `WorldID (u32)`
- `PosX/PosY/PosZ (s24 in 4-byte slots)`
- `Radius (s16)`
- `Intensity (s16)`
- `Red/Green/Blue (s16)`
- `Unknown_1E..Unknown_29 (u8[12])`

Unknown-byte pattern note:
- External observations report `Unknown_1E..Unknown_29` as often zero when `WorldID==0`, and in some non-zero-world records populated as incremental bytes (`0..11`).

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
