# RAHK

## Status
Documented, under-modeled.

## Purpose
AI/task hook stream.

## Record Forms
| Form | Bytes | Fields |
|---|---:|---|
| Base hook | `3` | `FunctionID (u16 le)`, `ParamCount (u8)` |
| Extended hook | `13` | Base fields + `Param1Header (u16)`, `Param1Value (u24)`, `Param2Header (u16)`, `Param2Value (u24)` |

## Notes
- Mixed variable-size records (no section-level record count).
- Boundaries come from RAHD (`RAHKOffset`, `RAHKLength`).
- External notes associate 3-byte hooks with simple links and 13-byte hooks with packed behavior args (`Wander`-style cases).
- Reported common IDs in corpus notes:
	- `0x0B00` on `CYRUS`/`GREMLIN` (`PlayerMain`-style behavior),
	- `0x1300` on `CAMERA` (`CameraController`-style behavior),
	- `0x2101` on NPC/enemy wander forms.
- Reported parameter semantics for 13-byte wander forms:
	- `Param1Value`: wander distance/range control,
	- `Param2Value`: delay/idle-time style control.
- Additional corpus constraints from external notes:
	- 3-byte form is concentrated on `CAMERA`, `CYRUS`, and `GREMLIN`.
	- 13-byte wander form commonly uses `Param1Header=0x0700`, `Param2Header=0x0700`.
	- `Param2Value` is often `0x800000` when a short idle pause is present.
- Runtime-impact note from external tests:
	- changing `CYRUS/GREMLIN` hook away from `PlayerMain` or `CAMERA` away from `CameraController` breaks expected control/follow behavior.

## Writer Notes
- Raw payload preserved (copy-window path).

## Unknowns
- `Param*Header` bit semantics.
- Complete `FunctionID` semantic map.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
- `../RGM_TASK_FUNCTION_RUNTIME.md`
