# PVO (Placement/Visibility Objects)

Last updated: 2026-03-11

## Supported Blocks
- `OCTH`
- `OCTR`
- `PLST`
- `MLST`
- `END`

## Scope
`*.PVO` appears to be a map-scoped placement/visibility container family in `maps/`.

## Retail GOG Validation (Install Corpus)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/maps`

Observed files:
- `CATACOMB.PVO`
- `CAVERNS.PVO`
- `DRINT.PVO`
- `ISLAND.PVO`
- `PALACE.PVO`

This confirms `PVO` is a core world-data family, not a one-off or editor-only artifact.

## Current Structural Model
- Chunked/tagged container using 4CC + big-endian length framing.
- Retail corpus order is stable across all sampled map files:
	- `OCTH -> OCTR -> PLST -> MLST -> END `
- All sampled files parsed cleanly with this framing model.

Representative chunk sizing (`ISLAND.PVO`):
- `OCTH`: `52` bytes @ `0x00000000`
- `OCTR`: `213,134` bytes @ `0x0000003C`
- `PLST`: `1,053,725` bytes @ `0x000340D2`
- `MLST`: `216,378` bytes @ `0x001354F7`
- `END `: marker @ `0x0016A239`

## Blocks

### OCTH
- Status: partially decoded.
- Observed length is consistently `52` bytes in sampled files.
- Likely role: octree header / root-level spatial metadata.
- First dword after section length appears to act as a key count/control value in sampled data (`0x0000000A` in `ISLAND.PVO`), but field semantics are still unresolved.

### OCTR
- Status: identified, structurally bounded by chunk framing.
- Likely role: octree node/region records.

### PLST
- Status: identified, structurally bounded by chunk framing.
- Likely role: placement list entries for map objects or references.

### MLST
- Status: identified, structurally bounded by chunk framing.
- Likely role: model/material/reference list used by placement records.

### END
- Status: identified terminator.

## Tooling Guidance
- Parser strategy should preserve unknown chunk payloads verbatim.
- Prioritize read-only chunk inventory + offset/length indexing before field-level decode.
- Defer writer support until round-trip byte parity is verified.

## Open Questions
- Exact field schema for `OCTH` and `OCTR` node layout.
- Whether `PLST` records reference `RGM`/`ROB` entities directly or via indirection.
- Coordinate basis/scaling and any map-origin conventions.

## Confidence
- High: corpus presence, map scoping, stable top-level chunk order, and section framing.
- Medium: probable block roles (`OCTH/OCTR/PLST/MLST`).
- Low: per-tag field semantics and cross-record linkage meaning.

## Sources
- Retail corpus: `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/maps/*.PVO`
- Repository reverse-engineering notes in `src/games/redguard/formats/`
