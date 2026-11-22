# Redguard Shared/Container Notes (External Reference)

Last updated: 2026-03-08

## Shared-with-Other-XnGine Notes (External Reference)
- `COL`: shared with Arena and others.
- `WLD`: shared with Daggerfall and others.
- `3D`: shared broadly.
- `3DC`: shared with most titles.

## Implementation Notes
- `WLD` now has a parser-derived working spec in `WLD.md` (Unity importer source-backed), including quadrant assembly and texture/rotation bitfield interpretation.
- Multiple Redguard container families now have parser-backed chunk framing notes (`RGM`, `SFX`, `GXA`, `FNT`, `TEXBSI`, `SAV`), but unknown-field semantics remain uneven.
- `ROB` and `TEXBSI` remain documentation-first in the current workspace; they are tracked as semantic/container references, not fully native Redguard C++ parsers.

## Redguard-Unique or Divergent Notes (External Reference)
- Redguard uses a distinct block/container ecosystem for several assets.
- Reported to share one container family across:
  - `ROB`, `FNT`, `PVO`, `RGM`, `SAV`, `TSG`, `GXA`, `LBM`
- Redguard `FNT` is explicitly not the same as "classic" FNT variants.

## Cross-Format Pattern Observations (Current)
- Frequent 4CC-tagged section framing with explicit length fields.
- Common use of `END ` style section/file terminators.
- Mixed-endian layouts occur in several formats (`RGM`, `TEXBSI`, `RTX`), so per-field endianness must be treated format-specifically.
- Unknown-field preservation is critical for safe round-trip editing.

## Unity Reader Primitive Notes
`RGFileImport/RGGFXImport/MemoryReader.cs` is the shared primitive used by many Unity parser classes:
- Scalar reads (`ReadInt16/ReadInt32/ReadUInt16/ReadUInt32`) use host/little-endian `BitConverter` semantics.
- Big-endian section-size fields are handled by explicit `ReverseBytes(...)` at call sites, not by reader-global endian mode.
- `Seek(adr, ofs)` uses absolute+offset addressing and enforces bounds (`Position <= Length`) with exception on overflow.
- Many parser sections therefore combine strict bounds checks with format-specific endian conversion at field level.

## Tooling/Integration Notes (External Reference)
- INI registration is useful and already integrated.
- Additional Redguard-specific tools desired (dialog/script editors).
- BSA-tool integration pending external info.

## Confidence Snapshot
- High confidence: high-level container identities, major block/tag inventories for core formats.
- Medium confidence: many per-section field names in parser-backed docs.
- Low confidence: deep semantics for several support families (`SWG/SWX`, Miles stack internals, portions of save chunks).

