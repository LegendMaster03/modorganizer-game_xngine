# MPSO

## Status
Documented, partially modeled.

## Purpose
Static object instances.

## Record Model
- Record size: `66` bytes with section-level `RecordCount`.

## Field Highlights
- Signed-24 positions in 4-byte slots.
- 3x3 transform matrix stored as `Q4.28` fixed-point (`int[9]`).
- Runtime conversion typically converts matrix entries to float before Euler extraction.
- External corpus notes suggest practical matrix values are constrained in sampled maps despite wider theoretical fixed-point range.

## Field Snapshot (record size 66)
- `Unknown_00 (u32)`
- `ModelName (char[8])`
- `Unknown_0C (u32)`
- `PosX/PosY/PosZ (s24 in 4-byte slots)`
- Matrix `XX YX ZX XY YY ZY XZ YZ ZZ` (`s32 Q4.28`)
- `Unknown_40 (u16)`

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.
