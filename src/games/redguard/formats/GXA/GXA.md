# GXA (Redguard Graphics Container)

Last updated: 2026-03-11

## Scope
`*.GXA` files are Redguard 2D graphics/image containers used heavily in `system/` UI and menu assets.
They are palette-indexed bitmap containers (not audio/model/save formats).

## Implementation Status
- Native C++ parser implemented in:
  - `src/games/redguard/redguardsgxaformat.h`
  - `src/games/redguard/redguardsgxaformat.cpp`
- Retail validation status:
  - `65/65` installed Redguard `.GXA` files parsed successfully via `redguardgfxscan`

## Retail GOG Validation (Install Corpus)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/system/*.GXA`

Observed:
- `65` `.GXA` files in retail `system/`.
- Representative files include `GUI.GXA`, `VIEWBACK.GXA`, `STMAP*.GXA`, `SUNSET.GXA`, `ISLAND.GXA`.

## Container / File Layout
Top-level chunked structure (big-endian chunk sizes):
1. `BMHD`
2. `BPAL`
3. `BBMP`
4. `END `

Common framing for chunks:
- `Tag` (`char[4]`)
- `Length` (`be u32`)
- `Payload` (`Length` bytes)

Practical parser note:
- `GUI.GXA` shows a `BBMP` length anomaly versus most files (details below), so parser paths should treat `BMHD` image-count + bounds checks as authoritative for image iteration.

## Blocks

### BMHD
- Status: documented
- Observed size: `34` bytes in sampled retail files
- Payload layout:
  - `char[22]` `Title`
  - `u8[10]` `Unknown`
  - `u16 le` `ImageCount`
- Notes:
  - `Title` is null-terminated or null-padded ASCII in observed files.
  - Example in `GUI.GXA`: `"GXlib image conversion"`.

Corpus image-count distribution (from 65 files):
- Most common: `1` image (`50` files)
- Multi-image examples observed: `2, 5, 6, 10, 20, 25, 32, 46, 66, 81`

### BPAL
- Status: documented
- Observed size: `768` bytes
- Model: 256 RGB triplets (`u8,u8,u8`), consistent with XnGine-style indexed palette payloads.
- Practical behavior: palette path aligns with other Redguard indexed-image families (`FNT`, `TEXBSI`, `COL` interactions).

### BBMP
- Status: documented
- Role: image bitmap records payload block.
- Per-image common header:
  - `s16 le` `Unknown0`
  - `u16 le` `Width`
  - `u16 le` `Height`
  - `s16 le[6]` `UnknownFields`
- The third `UnknownFields` value acts as an image-payload subtype marker in the retail corpus:
  - `0`: raw indexed image data follows immediately; payload size is `Width * Height`
  - `1`: encoded image payload; record boundaries are found by scanning for the next type-1 image header
  - `2`: encoded image payload; observed only in single-image files (`INVBACK.GXA`, `INVMASK.GXA`)

Retail subtype distribution:
- `Subtype 0`:
  - the standard case used by most `.GXA` files
- `Subtype 1`:
  - `GXICONS.GXA`
  - `PICKBLOB.GXA`
  - `SNUFF.GXA`
- `Subtype 2`:
  - `INVBACK.GXA`
  - `INVMASK.GXA`

Length-field observation:
- In most sampled files, `BBMP.Length` matches `END` offset minus BBMP payload start (normal chunk-length behavior).
- `GUI.GXA` and `GXICONS.GXA` are known outliers where `BBMP.Length` equals absolute `END` offset value instead.
- Recommendation:
  - treat `BBMP.Length` as advisory,
  - accept the absolute-END-offset anomaly,
  - prefer `BMHD.ImageCount` and subtype-specific record rules as authoritative.

### END
- Status: documented
- Marker: `END `

## Sample Header Evidence (`GUI.GXA`)
First bytes:
- `BMHD 00 00 00 22 ...`
- BMHD payload contains text `GXlib image conversion` + trailing count `0x0019` (`25`)
- followed by `BPAL 00 00 03 00` (`768`)
- followed by `BBMP ...`
- file tail ends with `45 4E 44 20` (`END `)

## Current Gap Summary
- Known-good:
  - chunk order (`BMHD -> BPAL -> BBMP -> END `),
  - `BMHD` title/unknown/image-count layout,
  - palette block size/role,
  - BBMP per-image common header,
  - BBMP raw vs encoded subtype split,
  - normalization policy for `GUI.GXA` / `GXICONS.GXA` length anomalies.
- Unknown/partial:
  - decompression semantics for subtype `1` and subtype `2` payloads,
  - semantic labeling of many BMHD/BBMP unknown fields.

## Reverse-Engineering Priorities
1. Decode subtype `1` and subtype `2` encoded image payloads into raster form.
2. Semantically label BMHD/BBMP unknown fields.
3. Decide whether writer support is needed for encoded subtype files or only raw subtype `0`.

## Confidence
- High: file family role, top-level chunk sequence, palette block usage, retail corpus presence, BMHD layout, and BBMP common header layout.
- Medium: subtype `1` header-scan record recovery and subtype `2` single-image encoded handling.
- Low/Medium: encoded payload decompression semantics.

## Related Documents

- `README.md`
- `DOC_GAPS_BACKLOG.md`
- `FNT.md`
- `../TEXBSI/TEXBSI.md`
- `../COL.md`
- `../LBM.md`
