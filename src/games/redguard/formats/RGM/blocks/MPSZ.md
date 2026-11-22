# MPSZ

## Status
Documented, partially modeled.

## Purpose
Object scaling/extents data.

## Record Model
- Record size: `49` bytes.
- Unity derives count as `sectionLength / 49`.

## Field Highlights
- Starts with mode/flag-like byte (`Unknown_00`).
- Carries size triplets (`Size*`, `PSize*`, `MSize*`).
- Includes signed position tuple (`PosX`, `PosY`, `PosZ`).

## Field Snapshot (record size 49)
- `Unknown_00 (u8)`
- `SizeX/SizeY/SizeZ (u32)`
- `PosX/PosY/PosZ (s32)`
- `PSizeX/PSizeY/PSizeZ (u32)`
- `MSizeX/MSizeY/MSizeZ (u32)`

Observed notes:
- `Unknown_00` is commonly zero in sampled maps.
- External notes report `PosX/PosY/PosZ` often zero in map defaults (semantic role unresolved).
- `PSize*`/`MSize*` naming remains uncertain (retained as neutral labels pending engine-level confirmation).

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.
