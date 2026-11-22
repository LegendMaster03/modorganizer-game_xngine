# BHDR

## Status
Documented, partially unresolved.

## Purpose
Primary image metadata record (dimensions/frame metadata + unknown fields).

## Layout
- Subrecord tag: `BHDR`
- Payload size: `26`

## Field Matrix
| Type | Observed values | Notes |
|---|---|---|
| `s16` | various | `XOffset` |
| `s16` | various | `YOffset` |
| `s16` | various | `Width` |
| `s16` | various | `Height` |
| `u8` | `0`, `1` | `1` often on animated effects (fire/water families) |
| `u8` | `0`, `1`, `9` | unknown |
| `s16` | `0` | unknown/expected zero |
| `s16` | `0` | unknown/expected zero |
| `s16` | `1..16` | `FrameCount` |
| `s16` | `0..500` (mostly `71`) | unknown |
| `s16` | `0` | unknown/expected zero |
| `s16` | `0` | unknown/expected zero |
| `u8` | `0`, `128`, `163` | `128` observed on fish; `163` on flame images |
| `u8` | `0`, `1`, `2` (mostly `1`) | unknown |
| `s16` | `0`, `4` (mostly `0`) | unknown |

## Notes
- C++ parser struct (`redguard_image_header_t`) confirms stable 26-byte layout.
- Several non-dimension fields remain unlabeled and are a top RE target.

## Unknowns
- Full semantics for BHDR unknown fields (`Unknown1..Unknown6` and related flags).
