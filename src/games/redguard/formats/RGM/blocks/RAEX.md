# RAEX

## Status
Documented, under-modeled.

## Purpose
Extra combat/visual/object metadata.

## Record Model
- Fixed record size: `30` bytes.
- Unity count derivation: `sectionLength / 30`.

## Key Fields
| Offset | Type | Field |
|---|---|---|
| `0x00` | `s16` | `Grip0` |
| `0x02` | `s16` | `Grip1` |
| `0x04` | `s16` | `Scabbard0` |
| `0x06` | `s16` | `Scabbard1` |
| `0x08` | `s16` | `Unknown08` |
| `0x0A` | `u16` | `TextureID` |
| `0x0C` | `s16` | `Vertex` |
| `0x0E` | `s16` | `Size` |
| `0x10` | `s16` | `TauntID` |
| `0x12` | `s16` | `Unknown12` |
| `0x14` | `s16` | `Unknown14` |
| `0x16` | `s16` | `Unknown16` |
| `0x18` | `s16` | `RangeMin` |
| `0x1A` | `s16` | `RangeIdeal` |
| `0x1C` | `s16` | `RangeMax` |

## Notes
- `TauntID` is often interpreted alongside RAAT `num_taunts` behavior.
- Raw payload preserved by Java writer path.
- External corpus notes report practical one-to-one alignment between RAEX records and RAHD headers in sampled maps.
- External correlation notes (needs broader corpus validation):
	- `Unknown08` often aligns with RAHD `Unknown_99`,
	- `TextureID` often aligns with RAHD texture-related field,
	- `Unknown14` often aligns with RAHD `Unknown_9B`,
	- `Unknown16` often aligns with RAHD `Unknown_9D`.
- Reported behavior note: for some large enemies (`dragon`, `golem`, `serpent`), `Range*` values may be interpreted with `*256` scaling.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
