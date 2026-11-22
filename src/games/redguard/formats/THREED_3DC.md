# 3D / 3DC (Redguard-Specific Deltas)

Last updated: 2026-03-11

## Scope
This file intentionally records only Redguard-relevant implementation deltas and reverse-engineering notes.

Shared/general 3D/3DC structure (cross-XnGine) is treated as baseline and not repeated here.

UESP alignment note (2026-03-11):
- an external excerpt with full baseline 3D/3DC structure tables was reviewed,
- this canonical document remains intentionally delta-focused (Redguard-specific behavior differences) rather than duplicating full generic container/section documentation.

## Baseline: 3D / 3DC Structure (UESP excerpt)

The following baseline layout and field descriptions are taken from the merged UESP excerpt and provide the canonical, cross-XnGine structure that `THREED_3DC.md` references. Redguard-specific deltas above should be read on top of these definitions.

Typical file layout (little-endian):

- Header (64 bytes)
- Face Data
- Vertex Coordinate Data
- Face Normal Data
- Animation Frame Data
- Section 4 Data (optional)
- UV Offset Data (optional)
- UV Coordinate Data

Header (64 bytes)

- byte Version[4]
- dword NumVertices
- dword NumFaces
- dword Radius
- dword NumFrames
- dword OffsetFrameData
- dword NumUVOffsets1
- dword OffsetSection4
- dword Section4Count
- dword UnknownZero
- dword OffsetUVOffsets
- dword OffsetUVData
- dword OffsetVertexCoors
- dword OffsetFaceNormals
- dword NumUVOffsets2
- dword OffsetFaceData

Notes on header fields:
- `Version` — 4 ASCII characters (no NUL). Examples: "v5.0", "v4.0", "v2.7".
- `NumVertices` — number of entries in the vertex coordinate list.
- `NumFaces` — number of face records (faces may be polygons with >3 verts).
- `Radius` — unknown; possibly bounding/collision radius.
- `NumFrames` — animated-frame count. For many `.3D` files this is `1`.
- `OffsetFrameData` — file offset to the frame data area.
- `NumUVOffsets1` / `NumUVOffsets2` — counts for the UV offset table; often equal.
- `OffsetSection4` / `Section4Count` — optional unknown-purpose section; absent when offset==0 or count==0.
- `OffsetUVOffsets` / `OffsetUVData` — offsets into UV structures; may be 0 in some `.3DC` variants.
- `OffsetVertexCoors` — offset to vertex coordinate list.
- `OffsetFaceNormals` — offset to face normal list.
- `OffsetFaceData` — typically `0x40` (immediately after the 64-byte header).

Face Data

A sequence of `NumFaces` variable-size records. Each face record:

- byte `VertexCount`
- byte `Unknown1`
- dword `Unknown2`
- dword `Unknown3`
- `VertexData[VertexCount]`

Each `VertexData` entry (8 bytes):
- dword `VertexIndex` (0-based index into vertex coordinate list)
- dword `Flags` (unknown semantics)

Vertex Coordinate Data

- `NumVertices` records of three 32-bit signed little-endian integers:
- int32 `X`
- int32 `Y`
- int32 `Z`

Face Normal Data

- `NumFaces` records of three 32-bit signed integers (X,Y,Z). Observed magnitudes around +/-256.

Animation Frame Data

- Present when `NumFrames` > 0. For files where `NumFrames == 1` this block is commonly 16 bytes. Exact format is not fully documented here; see Redguard deltas for importer heuristics.

Section 4 Data

- Unknown structured data. Present only when `OffsetSection4` != 0 and `Section4Count` > 0.

UV Offset Data

- Present in many `.3D` files. A table of `dword` offsets pointing into the UV coordinate area:
- `dword UVOffsets[NumUVOffsets2]`

UV Coordinate Data

- A sequence of float triplets (float X, float Y, float Z) used for UV/UVW coordinates.
- In `.3D` files the record count typically equals the number of offsets in the UV offset table; in `.3DC` files it is often equal to `NumVertices`.
- Values typically sit in a roughly `-1..1` range (assumed texture coordinates).

Caveats
- All values are little-endian.
- Some header fields and face/vertex flags remain undocumented; Redguard-specific files may require heuristics described in the deltas above.

## Redguard-Relevant Deltas

### Texture payload interpretation used in Redguard importer paths
- Unity Redguard importer applies packed `textureData` decode rules that are critical for Redguard assets:
  - `> v2.7`: solid-color sentinel when `(textureData >> 20) == 0x0FFF`, with color index from bits `15:8`.
  - `<= v2.7`: `textureId = textureData >> 7`, and `textureId < 2` treated as solid-color form.
- This logic is operationally important for Redguard map/object appearance reconstruction.

### Unity Mesh-Store Transform Conventions (`RG3DStore`)
- Unity mesh-store path applies additional model-space conversion constants on top of raw parser values:
  - `MESH_SCALE_FACTOR = 1 / 5120.0`
  - vertex-axis flip vector: `(1, -1, 1)`
  - rotation flip vector: `(-1, 1, -1)`
- Flat mesh helper builds a unit quad with fixed normal `(0,0,-1)` and UV winding baked in importer code.

Importer assumption note:
- Unity store comments state an explicit working assumption that direct explicit loads target `.3DC`, while many `.3D` assets are expected through ROB segment paths.

### Legacy 3DC compatibility behavior exercised on Redguard assets
- Redguard tooling paths include extra recovery logic for older `v2.6/v2.7`-style assets:
  - legacy header reinterpretation path (`ConvertHeader27()` in C++ parser family),
  - alternate vertex-offset attempt (`m_UseAltVertexOffset` path),
  - retry/heuristic path when coordinate ranges look invalid.

C++ struct-level model details:
- `rg_3dfile_header_t` uses a fixed 64-byte header with explicit slots for:
  - version tag, vertex/face/frame counts,
  - frame/UV/vertex/normal section offsets,
  - duplicated UV-offset count fields (`NumUVOffsets`, `NumUVOffsets2`),
  - legacy/unknown slots (`Unknown4`, section-4 offset/count).
- Transform constants used by loader/export path:
  - `COOR_TRANSFORM_FACTOR = 256.0`
  - `NORMAL_TRANSFORM_FACTOR = 256.0`
  - `UV_TRANSFORM_FACTOR = 4096.0`

Concrete C++ details from `Redguard3dFile`:
- version tags are normalized to numeric forms (`v2.6 -> 26`, `v2.7 -> 27`, `v4.0 -> 40`, `v5.0 -> 50`),
- for `<= v2.7`, `ConvertHeader27()` remaps header fields so legacy UV-related slots are interpreted through alternate offsets (`m_OffsetUVZeroes`, `m_OffsetUnknown27` capture original values),
- old-3DC vertex retry uses coordinate threshold constant `MAX_COORVALUE_FOROLD3DCRELOAD = 1090519040`; if min/max coordinate bounds exceed this range, loader retries vertex read with alternate offset mode,
- source comments explicitly mark this retry as partial/non-universal (known to help only some assets).
- face record decode is version-split:
  - for `> v2.7`, `U1` is read as `u16` and `U3` as `u8`,
  - for `<= v2.7`, `U1` is effectively read as one byte and `U3` is forced to `0`.
- for `<= v2.7`, per-face `VertexIndex` values are converted from byte offsets to indices via `/ 12`.
- UV coordinate count is derived from section span (`(nextOffset - OffsetUVData) / 12`) instead of header counters.

Additional implementation caveats in this C++ toolchain:
- `LoadHeader()` calls `ConvertVersion()` but does not branch on its return value; unknown version handling depends on downstream parse failure behavior.
- Section sizing relies on `FindNextOffsetAfter()` using the minimum larger header offset, so malformed/overlapping offsets can skew inferred section extents.

### Alternate UnknownData layout seen in Redguard-focused Unity `io_tst`
- Unity alternate parser (`io_tst/3dctst/RG3DFile.cs`) supports two forms for `offsetUnknownData` content:
  - direct contiguous `numVertices * (x,y,z)` triples,
  - offset-table driven triples (`offsetUnknownOffsets` indirection).
- Parser comments suggest this may carry normal-like vectors for some Redguard assets (for example `MKCRATE*`), but this remains unconfirmed.

### Frame metadata ambiguity relevant to Redguard animation decoding
- Unity alternate parser records per-frame tuple:
  - `frameVertexOffset`, `frameNormalOffset`, `u1`, `u2`.
- Observed exploratory notes:
  - `u2 == 2` appears to imply `4 bytes per face` at `frameNormalOffset`.
  - `u2 == 4` and `u2 == 8` remain unresolved.
- This is kept as a Redguard investigation target, not a canonical engine rule.

## Redguard Validation Corpus Signals (Unity `io_tst`)
- `io_tst/3dctst/dumpinfo.cs` enumerates a large Redguard model corpus (`*.3D`/`*.3DC`) for parser stress-testing.
- Practical value: confirms the above heuristics were derived while targeting many real Redguard files, not isolated synthetic cases.

## C++ Harness Behavior (`3DFileTest.cpp`)
The C++ extraction harness in `redguard-3dfiletest` performs corpus-wide sweeps with exporter wiring:
- enumerates all `*.3D` and `*.3DC` files under `fxart`
- loads each through `CRedguard3dFile::Load(...)`
- exports FBX per file (`SaveAsFbx(...)`)

Texture-side harness path:
- loads default palette from `REDGUARD.COL`
- enumerates `TEXBSI.*`
- loads via `CRedguardTexBsiFile` and exports decoded frames/images to PNG

Practical implication:
- implementation notes in this repo are backed by batch-processing behavior across full retail-like corpora, not single-file probes.

## Confidence
- High: importer-side texture-data decode behavior and legacy compatibility branches exist and are active in Redguard tooling.
- Medium: unknown-data semantics and frame `u2` meaning remain hypotheses.

## Sources
- `redguard-3dfiletest-master/3DFileTest/Common/Redguard3dFile.h`
- `redguard-3dfiletest-master/3DFileTest/Common/Redguard3dFile.cpp`
- `redguard-3dfiletest-master/3DFileTest/3DFileTest.cpp`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RG3DFile.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/3dctst/RG3DFile.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/3dctst/dumpinfo.cs`
