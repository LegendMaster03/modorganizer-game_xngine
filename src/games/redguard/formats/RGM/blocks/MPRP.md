# MPRP

## Status
Documented, partially modeled.

## Purpose
Rope definitions.

## Record Model
- Record size: `80` bytes with section-level `RecordCount`.

## Field Highlights
- Signed-24 position slots (`PosX`, `PosY`, `PosZ`).
- Behavior fields include `YAngle`, `Type`, `Swing`, `Speed`, `Length`.
- Includes fixed-name IDs: `StaticModel`, `ModelName`.
- Tail fields (`Unknown_34..Unknown_4C`) remain opaque.
- External behavior notes:
	- rope anchor coordinates represent top mount point,
	- reported `Type` values include `0`, `1`, and `18` (with `18` used in attached-static-object cases),
	- `Swing`/`Speed` modulate rope movement behavior.

## Field Snapshot (record size 80)
- `Unknown_00 (u32)`
- `Unknown_04 (u8)`
- `PosX/PosY/PosZ (s24 in 4-byte slots)`
- `YAngle (u32)`
- `Type (u32)`
- `Swing (u32)`
- `Speed (u32)`
- `Length (u16)`
- `StaticModel (char[9])`
- `ModelName (char[9])`
- `Unknown_34 (u32)`
- `Unknown_38 (u32)`
- `Unknown_3C (u32)`
- `Unknown_40 (u32)`
- `Unknown_44 (u32)`
- `Unknown_48 (u32)`
- `Unknown_4C (u32)`

Unknown-tail pattern notes:
- External corpus notes report recurring triplets in `Unknown_34..Unknown_3C` such as `62/62/63` and `62/62/62` (plus map-specific zeroed cases).
- `Unknown_40..Unknown_4C` are commonly zero in sampled maps.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
