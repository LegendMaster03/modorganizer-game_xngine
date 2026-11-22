# MPOB

## Status
Documented, partially modeled.

## Purpose
Scriptable/mobile object instances.

## Record Model
- Record size: `66` bytes with section-level `RecordCount`.

## Field Highlights
- Signed 24-bit positions in 4-byte slots (`PosX`, `PosY`, `PosZ`).
- Packed texture field split: `textureId = value >> 7`, `imageId = value & 0x7F`.
- Rotation/angle fields use 2048-angle unit convention in runtime conversion (`180/1024`).
- `TypeID` follows ObjectType enum (`1` 3D object, `2` audio, `3` flat, `4` light object, `6` light-style object).
- `ScriptName` links instances to RAHD script headers.
- Light-related fields (`Intensity`, `Radius`, `RGB`) are type-dependent in behavior.
- `ViewPitch/ViewYaw/ViewRoll` use Bethesda-style 2048-angle convention (common decode pattern includes `value & 2047`).
- `ModelName` is fixed `char[9]` and can be truncated relative to full model filename conventions.
- `WorldID` is usually map-local, with known multi-world usage on `ISLAND` variants.

## Field Snapshot (record size 66)
- `Unknown_00 (u32)`
- `TypeID (u8)`
- `IsActive (bool)`
- `ScriptName (char[9])`
- `ModelName (char[9])`
- `IsStatic (bool)`
- `Unknown_19 (u16)`
- `PosX/PosY/PosZ (s24 in 4-byte slots)`
- `ViewPitch/ViewYaw/ViewRoll (u32)`
- `TextureID/SubImageID (u16 packed)`
- `Intensity/Radius/ModelID/WorldID (s16)`
- `Red/Green/Blue (s16)`

Packed texture note:
- `SubImageID` counting follows animated/multi-frame indexing behavior and is not a simple absolute image ordinal in all tooling notes.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Related Docs
- `../RGM_ENUMS.md`
- `../RGM_ENUMS_NOTES.md`
- `../RGM_BEHAVIOR_NOTES.md`
