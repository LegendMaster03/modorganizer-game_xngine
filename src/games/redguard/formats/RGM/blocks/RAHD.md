# RAHD

## Status
Documented + partially implemented.

## Purpose
Master per-script/per-object index; drives slicing for `RASC`, `RAST`/`RASB`, `RAVA`, `RAAT`, animation data, and object-side arrays.

## Section Layout
| Offset | Type | Field | Notes |
|---|---|---|---|
| `+0x00` | `u32 le` | `HeaderCount` | Number of RAHD entries |
| `+0x04` | `u32 le` | `HeaderInit` | Common constant in corpus/tool path (`1B 80 37 00`) |
| `+0x08` | `HeaderCount * 165` | `HeaderRecords` | Fixed-size entries |
| `+...` | `u32 le` | `TrailingValue` | Observed in Unity parser; unresolved |

## Record Key Fields
| Record Offset | Type | Field | Notes |
|---|---|---|---|
| `0x04..0x0C` | `char[9]` | `ScriptName` | Null-trimmed |
| `0x0D` | `u16 le` | `InstanceCount` | Used with `MPOB` expansion |
| `0x41` | `u32 le` | `StringCount` | Header string count |
| `0x49` | `u32 le` | `RASBOffsetIndex` | String-offset index/offset |
| `0x4D` | `u32 le` | `RASCLength` | Bytecode length |
| `0x51` | `u32 le` | `RASCOffset` | Bytecode offset |
| `0x55` | `u32 le` | `RASCStartAt` | Script entry PC |
| `0x75` | `u32 le` | `LocalVarCount` | Local variable count |
| `0x7D` | `u32 le` | `RAVAOffset` | Variable payload byte offset |

## Extended Field Inventory (record size 165)
- `Unknown_00 (u32)`
- `Unknown_04 (u32)`
- `ScriptName (char[9], null padded)`
- `MPOB_Record (u32)`
- `Unknown_15 (u32)`
- `RANM_Length (u32)`
- `RANM_Offset (u32)`
- `RAAT_Offset (u32)`
- `RAAN_Record (u32)`
- `RAAN_Length (u32)`
- `RAAN_Offset (u32)`
- `RAGR_MaxAnimGroup (u32)`
- `RAGR_Offset (u32)`
- `Unknown_39 (u32)`
- `Unknown_3D (u32)`
- `Unknown_41 (u32)`
- `RASB_Record (u32)`
- `RASB_Length (u32)`
- `RASB_Offset (u32)`
- `RASC_Length (u32)`
- `RASC_Offset (u32)`
- `RASC_StartAt (u32)`
- `RAHK_Length (u32)`
- `RAHK_Offset (u32)`
- `RALC_Record (u32)`
- `RALC_Length (u32)`
- `RALC_Offset (u32)`
- `RAEX_Length (u32)`
- `RAEX_Offset (u32)`
- `RAVA_Record (u32)`
- `RAVA_Length (u32)`
- `RAVA_Offset (u32)`
- `Unknown_85 (u32)`
- `FrameCount (u32)`
- `MPSZ_Normal_ID (s32)`
- `MPSZ_Combat_ID (s32)`
- `MPSZ_Dead_ID (s32)`
- `Unknown_99 (u16)`
- `Unknown_9B (s16)`
- `Unknown_9D (s16)`
- `TextureID (u16)`
- `RAVC_Offset (u32)`

## Notes
- Fixed record size used by Java + Unity parser paths.
- Additional fields carry offsets/counts for `RAHK`, `RALC`, `RAEX`, `RAAN`, `RAGR`, `RANM`, optional `RAVC`, and MPSZ link IDs.
- External article value-pattern notes (pending full parity sweep):
	- `Unknown_00` is frequently `0x0037801B` for `MAPCOUNT`-like header entries and `0` for many other records, with reported map-specific exceptions.
	- `Unknown_3D`/`Unknown_41` are often zero and may represent an unused RAFS pointer trio in legacy layouts.
	- `Unknown_99`/`Unknown_9B`/`Unknown_9D` show NPC-class-correlated values in external notes and may pair with combat/behavior metadata.

## Unknowns
- Canonical semantics of several `unknown*` fields.
- Exact role of trailing `u32` after records.
