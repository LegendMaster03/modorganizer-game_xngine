# ROB File Format (Redguard)

Last updated: 2026-03-11

## Scope
The `ROB` files in Redguard's `fxart` directory are archives containing model records.  
Reference documentation states there are 31 such files (maps, inventory/menu models, and test archives).

## Workspace Implementation Status
- Dedicated Redguard C++ `ROB` reading support now exists in:
  - `src/games/redguard/redguardsrobformat.h`
  - `src/games/redguard/redguardsrobformat.cpp`
- Embedded segment payloads are parsed through the finished Redguard `3D`/`3DC` path.
- External-reference segments are resolved as `<SegmentName>.3DC` from the containing directory when possible.
- No writer/round-trip support exists yet.

Reported file-family composition (external docs):
- one ROB per map,
- one inventory-model ROB,
- one menu-model ROB,
- plus `PALATEST` and `TEMPTEST` test-model archives.

## Retail GOG Validation (Install Corpus)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/fxart`

Observed:
- `31` `.ROB` files.
- `fxart/` also contains large `.3DC` corpus, matching external-segment mode behavior.

## Format Description
Each `ROB` file is composed of:
1. A fixed header (20 bytes)
2. A sequence of segment records

Each segment either:
- embeds a `3D` payload directly, or
- references an external `3DC` model by name.

All values are little-endian unless explicitly noted otherwise.

## ROB File Layout
- `Header` (20 bytes)
- `Segments` (variable, repeated `NumSegments` times)
- `END ` footer (4 bytes)

## ROB Header Struct

| Name | Type | Description |
|---|---|---|
| `OARC` | `u8[4]` | ASCII `"OARC"` |
| `Unknown1` | `u32` | Unknown |
| `NumSegments` | `u32` | Number of segment records |
| `OARD` | `u8[4]` | ASCII `"OARD"` |
| `Unknown2` | `u32` | Unknown |

Header framing notes:
- `OARC` and `OARD` literals are treated as hard markers in reference implementations.
- Retail GOG validation confirms the actual header is 20 bytes:
  - `0x00..0x03` = `OARC`
  - `0x04..0x07` = `Unknown1`
  - `0x08..0x0B` = `NumSegments`
  - `0x0C..0x0F` = `OARD`
  - `0x10..0x13` = `Unknown2`

## ROB Segment Struct
Each segment begins with an 80-byte segment header.

| Name | Type | Description |
|---|---|---|
| `Unknown1` | `u32` | Unknown |
| `SegmentName` | `u8[8]` | Model name string |
| `Unknown2` | `u32` | Segment mode/flags field; do not currently treat as a strict enum |
| `Unknown3` | `u32[15]` | Unknown fields |
| `Size` | `u32` | Segment payload size; `0` for external-reference segments |
| `Data` | `u8[]` | Embedded `3D` bytes; empty for external-reference segments |

External docs describe each ROB as concatenated segment structures where each segment either embeds a complete `3D` payload or names a corresponding external `3DC` file.

Retail segment framing confirmed from corpus:
- first segment header starts immediately after the 20-byte ROB header
- each segment header is exactly 80 bytes
- `Size` is stored at `segment+0x4C`
- embedded payload begins at `segment+0x50`
- all retail GOG `ROB` files end with a trailing ASCII `END ` marker after the last segment

## Behavioral Notes
- If `Size > 0`, `Data` is a full embedded `3D` object.
- If `Size == 0`, the segment names a model that resolves to external `<SegmentName>.3DC` in `fxart`.
- `Unknown2` correlates with behavior but is not limited to `0` and `512` in retail files.

Unity importer-store behavior (`RG3DStore.LoadMeshIntermediatesROB`):
- For each ROB segment:
	- if `Size > 0`, Unity loads embedded segment `Data` through in-memory `RG3DFile.LoadMemory(...)`.
	- if `Size == 0`, Unity attempts external fallback load `<SegmentID>.3DC` from art folder.
- On external fallback failure, importer logs and continues (non-fatal per-segment failure behavior).

## Embedded/External 3D Payload Notes (C++ Parser)
`redguard-3dfiletest` parses shared 3D/3DC payloads used by ROB segments with:
- fixed `64`-byte model header (`rg_3dfile_header_t`),
- versions parsed: `v2.6`, `v2.7`, `v4.0`, `v5.0`,
- explicit offsets/counts for face data, face normals, frame data, UV offsets/data, and vertex coordinates.

Coordinate/UV normalization constants in parser:
- coordinates and normals: divide by `256.0`
- UVs: divide by `4096.0`

Important compatibility behavior in parser:
- For versions `<= v2.7`, face vertex indices are interpreted as byte offsets and converted to index (`/12`).
- Some older `.3DC` files require alternate vertex-offset fallback during load (`m_UseAltVertexOffset`).
- Loader includes an old-3DC retry heuristic (`m_TryReloadOld3dcFileVertex`) to handle legacy coordinate-offset anomalies.

These are payload parser details; ROB itself remains a segment archive wrapper.

## Parser Guidance
- Preserve unknown fields exactly during round-trip.
- Use `NumSegments` as authoritative loop bound.
- Treat `Size == 0` vs `Size > 0` as the reliable external/embedded discriminator.
- Preserve `Unknown2` as raw mode/flags data.

## Confidence + Outstanding Verification
- High confidence: ROB container framing, real 20-byte header, 80-byte segment headers, and embedded/external split via `Size`.
- Medium confidence: exact semantics of non-mode unknown header/segment fields.
- Confirmed retail mode values include `0`, `256`, `512`, and a small number of additional non-canonical values.
- Needs confirmation: exact semantics of the non-canonical `Unknown2` values.

## Related Format Docs
- `RGM/RGM.md` (map/object records that reference model assets)
- `TEXBSI/TEXBSI.md` (textures used by ROB/3D content)

## Source Provenance

External ROB notes confirm the 20-byte header and 80-byte segment structure. Unknown header/segment fields remain opaque until independently proven.

