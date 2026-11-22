# RAGR

## Status
Documented, under-modeled.

## Purpose
Animation groups and frame-control logic.

## Group Layout
| Offset | Type | Field |
|---|---|---|
| `+0x00` | `u16 le` | `DataLength` |
| `+0x02` | `u16 le` | `AnimGroup` |
| `+0x04` | `u16 le` | `FrameSpeed` |
| `+0x06` | `u16 le` | `AnimType` |
| `+0x08` | `u16 le` | `FrameCount` |
| `+0x0A` | `FrameCount * 3` | `Frames` |

## Frame Record (3 bytes)
| Bits / Byte | Field | Decode |
|---|---|---|
| `u16 bits 0..3` | `FrameType` | `frameRecord & 0x0F` |
| `u16 bits 4..14` | `FrameValue` | `(frameRecord >> 4) & 0x07FF` |
| `u16 bit 15` | `TimeScaleFlag` | `((frameRecord >> 15) & 1) != 0` |
| trailing `u8` | `ModifierValue` | Type-dependent |

## Enum Highlights
- `AnimType`: `0` normal, `1` combat/no-stop, `2` death/no-panic.
- Common `FrameType`: `0` ShowFrame, `1` EndAnimation, `2` GoToPrevious, `3` GoToFuture, `4` PlaySound, `5` BreakPoint, `11` Rumble, `15` Transition.
- Category notes from external enum docs:
	- `AnimGroup` IDs `0..168` are grouped into categories (idles, movement, jumps/falls, combat, rope, swim, speech, misc).
	- `AnimType=0` is commonly interruptible; `AnimType=1` favors graceful completion; `AnimType=2` suppresses panic fallback behavior.

## Parser Notes
- Unity reads raw payload via `RAHD.RAGROffset`.
- Scan terminates on sentinel `PeekInt16() == 0`.
- External behavior notes emphasize many groups contain control frames (not just display frames), and loops often rely on `GoToPrevious` / `GoToFuture`.
- `PlaySound` frames reference `SOUND/MAIN.SFX` IDs in runtime behavior models.

## Detailed FrameType Semantics (external notes)
- `FrameType 4` (`PlaySound`):
	- `FrameValue & 0xFF` commonly used as sound-effect ID,
	- higher bits can encode effect flags (for example tempo/stretch behavior),
	- `ModifierValue` is interpreted based on active flags.
- `FrameType 15` (`Transition`):
	- low bits of `FrameValue` represent action/transition flags,
	- higher bits can specify target starting frame behavior,
	- `ModifierValue` is used in target-group transition routing (exact mapping still unresolved).
- Looping behavior commonly uses `FrameType 2`/`3` control-frame pairs instead of only linear `ShowFrame` runs.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Unknowns
- Full in-engine semantics of `TimeScaleFlag` and `ModifierValue` by frame type.

## Related Docs
- `../RGM_ENUMS.md`
- `../RGM_ENUMS_NOTES.md`
- `../RGM_BEHAVIOR_NOTES.md`
