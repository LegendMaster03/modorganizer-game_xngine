# BBMP - Bitmap BitMaP image

## Status
Documented

## Purpose
- Holds the per-image records for `GXA`.
- Retail corpus contains three BBMP payload subtypes.

## Common Record Header
Each image record begins with:
- `0x00` `s16 le` `Unknown0`
- `0x02` `u16 le` `Width`
- `0x04` `u16 le` `Height`
- `0x06` `s16 le[6]` `UnknownFields`

Header size:
- `18` bytes total

## Subtype Marker
The third `UnknownFields` value (`UnknownFields[2]`) is a subtype marker in retail Redguard:
- `0`: raw indexed pixels
- `1`: encoded payload with record boundaries recovered by scanning for the next type-1 header
- `2`: encoded payload observed only in single-image files

## Subtype 0
- Image payload immediately follows the 18-byte header.
- Payload size is exactly `Width * Height`.
- This is the standard `GXA` image form used by most files.

## Subtype 1
- Seen in:
  - `GXICONS.GXA`
  - `PICKBLOB.GXA`
  - `SNUFF.GXA`
- After the 18-byte header, payload bytes continue until the next subtype-1 header or the end of `BBMP`.
- Common header signature observed in retail files:
  - `Unknown0 = 1`
  - `UnknownFields = [0, 0, 1, 0, 0, 0]`
- Encoded-payload semantics remain unresolved.

## Subtype 2
- Seen in:
  - `INVBACK.GXA`
  - `INVMASK.GXA`
- Observed only in single-image files.
- The 18-byte header is followed by the remaining `BBMP` payload.
- Common header signature observed in retail files:
  - `Unknown0 = 1`
  - `UnknownFields = [0, 0, 2, 0, 0, 0]`
- Encoded-payload semantics remain unresolved.

## Chunk-Length Anomaly
- `GUI.GXA` and `GXICONS.GXA` store `BBMP.Length` as the absolute `END ` offset instead of a normal payload length.
- Parser/writer code should tolerate this and derive the real payload span from the absolute end offset.

## Writer Notes
- For subtype `0`, write the 18-byte header followed by `Width * Height` indexed pixels.
- Do not normalize or rewrite subtype `1` / subtype `2` payloads without a fully understood encoder.
