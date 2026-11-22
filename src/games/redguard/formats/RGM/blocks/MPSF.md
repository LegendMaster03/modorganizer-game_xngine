# MPSF

## Status
Documented, partially modeled.

## Purpose
Flat texture objects.

## Record Model
- Record size: `24` bytes with section-level `RecordCount`.

## Field Highlights
- Signed-24 positions in 4-byte slots.
- Packed texture split: `textureId = value >> 7`, `imageId = value & 0x7F`.
- External note: subimage indexing behavior differs from some MPOB usage assumptions; treat MPSF subimage IDs as direct flat-record references in this section context.

## Field Snapshot (record size 24)
- `Unknown_00 (u32)`
- `Unknown_04 (u32)`
- `PosX/PosY/PosZ (s24 in 4-byte slots)`
- `TextureID/SubImageID (u16 packed)`

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.
