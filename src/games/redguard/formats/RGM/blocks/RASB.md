# RASB

## Status
Documented + partially implemented.

## Purpose
Offsets into `RAST` string pool.

## Record Model
- Record size: `4` bytes.
- Field: `RAST_Offset (u32)`.
- Values are interpreted little-endian.

## Relationships
- RAHD header fields define per-header window/usage.
- For headers with multiple strings, read sequential entries until `RAHD.RASB_Record` count is reached.
- Reused strings appear as separate RASB records that may share the same `RAST_Offset`.
- `RAHD.RASB_Offset` is the per-header starting index/offset into this table.
- `RASC` string references are header-local indices, not direct `RAST` offsets.

## Writer Notes
- Java path stores per-header string offsets using first-occurrence positions in serialized `RAST` stream.
- Workspace C++ writer follows the same policy.

## Notes
- `RASB` is the indirection layer that allows `RAST` de-duplication across headers.
- A malformed `RASB` offset can desynchronize script string resolution even when `RAST` is otherwise intact.
