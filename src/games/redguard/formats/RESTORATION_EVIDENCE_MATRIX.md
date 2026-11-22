# Restoration Evidence Matrix (Project Extraction)

Last updated: 2026-03-08

## Scope
This matrix records what the first two extracted projects **actually parse/use** versus what they preserve as opaque bytes.

Projects covered:
- `redguard-mod-manager-main` (Java)
- `redguard-3dfiletest-master` (C++)

Goal:
- isolate which unknown fields are still unresolved,
- avoid over-claiming semantics from parser structure alone,
- prioritize the next restoration validation tasks.

## RGM Unknowns / Partial Semantics

| Area | Java (`redguard-mod-manager`) evidence | What this proves | Remaining unknown |
|---|---|---|---|
| `RAHK` | `MapFile` does not decode hook records; section is copied raw during write (`copyRecordsToOutput(output, "RAHK", "RALC", "RAEX")`) | tool preserves bytes but does not model hook semantics | all per-hook field semantics |
| `RALC` | no direct field decode in `MapFile`; copied raw in same pre-attribute copy window | location table exists and round-trips raw | detailed per-record semantic labels remain external/inferred |
| `RAAT` | loaded as contiguous 256-byte slices per header (`Arrays.copyOfRange(records.get("RAAT"), i*256, (i+1)*256)`), then written back as opaque blobs | fixed 256-byte record size is implementation-backed | per-byte/field meanings mostly unresolved in this codebase |
| `RAVA` | decoded as LE `u32` stream; pointer from RAHD is byte offset divided by 4; writer prepends 4 zero bytes | variable-storage and indexing mechanics are implementation-backed | semantic meaning of individual variable slots is script-dependent |
| `RAVC` | omitted from Java writer section list | writer behavior is non-round-trip for optional `RAVC` | policy decision needed for full-fidelity writing |

## TEXBSI BHDR Unknowns

| Area | Evidence | What this proves | Remaining unknown |
|---|---|---|---|
| BHDR struct shape | C++ defines/reads full 26-byte `redguard_image_header_t` (`Unknown1..Unknown6` + zeros + dimensions/frame count) | byte layout and field widths are stable | semantic meaning of unknown BHDR fields |
| Runtime decode drivers | Java and C++ texture decode paths use `Width`, `Height`, `FrameCount`; unknown BHDR members are not used for decode branching | core frame extraction does not depend on unknown BHDR fields in these tools | whether game/runtime uses unknowns for timing/behavior/origin flags |
| Header preservation | Java rebuild path stores/re-emits combined header bytes (`getHeaderBytes()` + `output.write(texture.getHeaderBytes())`) | unknown BHDR bytes are preserved in round-trip writer path | unknowns still unlabeled despite preservation |

## Animation Offset Table Divergence (Implementation Caveat)

| Tool | Behavior |
|---|---|
| Java (`Texture`) | treats frame-offset table as `height * frameCount` entries of `u32` (byte array length `*4`) |
| Unity (`RGBSIFile`) | reads `height * frameCount` `Int32` entries (entry count model) |
| C++ (`RedguardTexBsiFile`) | `ReadAnimationData()` allocates `vector<dword>` of `height*frameCount` but reads only that many bytes via `fread(..., OffsetDataSize, ...)`, implying likely under-read bug |

Interpretation:
- format intent is still consistent with a `u32` per row-offset entry,
- C++ implementation appears to contain a byte-count bug in the animated path.

## Priority Queue (Next Restoration Validation Steps)
1. Validate `RAHK` record variants against runtime call sites (hook function IDs and parameter layouts).
2. Build `RAAT` behavioral map from script function usage (`SetAttribute`/`GetAttribute`) plus object-store consumers.
3. Resolve BHDR unknowns by correlation study across animated sets (fire/water/fish/flame families) and runtime render behavior.
4. Decide and implement explicit writer policy for optional `RAVC` round-trip.

## Evidence Sources
- `redguard-mod-manager-main/src/redguard/MapFile.java`
- `redguard-mod-manager-main/src/redguard/MapHeader.java`
- `redguard-mod-manager-main/src/redguard/TextureFile.java`
- `redguard-mod-manager-main/src/redguard/Texture.java`
- `redguard-3dfiletest-master/3DFileTest/Common/RedguardTexBsiFile.h`
- `redguard-3dfiletest-master/3DFileTest/Common/RedguardTexBsiFile.cpp`