# IMAGE_HEADER

## Status
Documented.

## Purpose
Per-image outer header used before TEXBSI subrecords.

## Layout
| Offset | Type | Name | Description |
|---|---|---|---|
| `+0x00` | `u8[9]` | `ImageName` | Null-terminated/trimmed name token |
| `+0x09` | `u32 le` | `ImageSize` | Size of image subrecord payload block (excludes 13-byte header) |

## Notes
- All-zero `ImageName` marks end of image stream.
- Local `TEXBSI.412` parse confirms 64 names `E12000..E12063`.
- Java writer emits trailing zero trailer after final image record (`u32 0`, `u32 0`, `u8 0`) in that implementation path.
