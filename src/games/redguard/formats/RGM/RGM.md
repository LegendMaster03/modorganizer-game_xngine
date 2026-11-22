# RGM File Format (Redguard)

Last updated: 2026-03-29

## Scope
RGM files in `maps` contain static objects, dynamic objects, and map scripts.

## Container / File Layout
Repeated sections until `END `:
- `SectionName` (`char[4]`)
- `DataLength` (`be u32`) excluding section name field
- optional `RecordCount` (`u32`) for section types that use it
- section payload

Notes:
- Sections are reported to appear in fixed order.
- Empty sections may still be present.
- `RAVC` is optional and absent when unused.
- External corpus notes report `RAFS` is effectively empty in sampled maps.
- Java parser/writer scanner behavior:
  - reads repeated `Tag[4] + Length[be u32] + Payload` until `END `,
  - preserves unknown section tags for write-back.

## Common Header (Section Framing)
| Offset | Type | Name | Description |
|---|---|---|---|
| `+0x00` | `char[4]` | `SectionName` | 4CC section tag (`RAHD`, `MPOB`, ..., `END `) |
| `+0x04` | `be u32` | `DataLength` | Section payload size in bytes (`END ` has no size field in some parser paths) |
| `+0x08` | `u32` (optional) | `RecordCount` | Present for count-based sections (for example `MPOB`, `MPSO`) |

## Section / Block Order
1. `RAHD`
2. `RAFS`
3. `RAST`
4. `RASB`
5. `RAVA`
6. `RASC`
7. `RAHK`
8. `RALC`
9. `RAEX`
10. `RAAT`
11. `RAAN`
12. `RAGR`
13. `RANM`
14. `RAVC` (optional)
15. `MPOB`
16. `MPRP`
17. `MPSO`
18. `MPSL`
19. `MPSF`
20. `MPMK`
21. `MPSZ`
22. `WDNM`
23. `FLAT`
24. `END `

Writer behavior notes:
- Legacy Java mod-manager path emits:
  - `RAHD, RAFS, RAST, RASB, RAVA, RASC, RAHK, RALC, RAEX, RAAT, RAAN, RAGR, RANM, MPOB, MPRP, MPSO, MPSL, MPSF, MPMK, MPSZ, WDNM, FLAT, END `,
  - `RAVC` is not emitted even if present in source.
- Workspace C++ writer path now preserves optional `RAVC` presence/absence:
  - emits `RAVC` only when the source map contains `RAVC`.

## Blocks
- Per-block deep content is maintained in `blocks/*.md`.
- Use `README.md` in this folder for ordered navigation.
- Minimal summaries:
  - `RA*` blocks: script/runtime header, strings, variables, hooks, attributes, animations, optional collision metadata.
  - `MP*` blocks: instantiated objects, ropes, statics, lights, flat-objects, markers, scaling/extents.
  - `WDNM`: walk-node graph data.
  - `FLAT`: currently unresolved section role.
  - `END`: stream terminator.

## Cross-Block Relationships
- `RAHD` is the main index hub: it links names/counts/offsets/lengths across `RASC`, `RAST`/`RASB`, `RAVA`, `RAAT`, `RAAN`, `RAGR`, `RANM`, `RALC`, `RAEX`, and optional `RAVC`.
- `RAST` and `RASB` form a paired string system (`RASB` offsets into concatenated null-terminated `RAST`).
- `RAVA` is variable storage addressed by RAHD byte offsets (typically interpreted as `u32` index via `/4`).
- `RAAT` attribute slices are consumed per object/script index (common runtime convention: contiguous 256-byte windows).
- `RALC` location offsets are applied relative to base object positions (`MPOB` entries).

## Coordinate Encoding Note
- Coordinates in multiple sections are documented as signed 24-bit values, sometimes stored in 4-byte slots.
- External notes emphasize field-dependent width variability (3-byte vs 4-byte allocation) with equivalent signed interpretation in many paths.
- Unity runtime conversion conventions (implementation-specific):
  - world/object position scale commonly uses `1/5120.0`.
  - MPOB angle conversion commonly uses `180/1024`.

## Discrepancies / Competing Theories
- Theory A: writer should preserve optional sections exactly (including `RAVC`) for round-trip safety.
- Current evidence: parser/runtime paths treat `RAVC` as optional but real; external corpus notes show maps where it is present and meaningful.
- Current workspace policy: preserve optional sections when present; `RAVC` omission is considered a legacy Java writer limitation, not canonical format behavior.

## External Documentation Merge Highlights (2026-03-08)
- Merged high-signal section-role and field-behavior notes from external written docs.
- Preserved/confirmed:
  - optional/missing `RAVC` behavior,
  - signed-24 coordinate nuances,
  - `RAGR` control-frame emphasis (`PlaySound` linkage, transition semantics),
  - `WDNM` hierarchy framing (`WalkMap -> WalkNode -> NodeRoute`).

## UESP Merge Highlights (2026-03-11, external excerpt)
- Added/confirmed alignment with a detailed UESP-style RGM reference excerpt.
- High-signal confirmations preserved in canonical block docs:
  - section order and record-size expectations,
  - `MPMK` trailing marker-enable footer bytes,
  - signed-24 coordinate usage in 3/4-byte slots,
  - strong `RAVC` optionality signal (reported concentrated usage in `CATACOMB`/`DRINT`).
- Large enum and attribute tables from the excerpt are retained as external-reference material and should be treated as implementation guidance pending byte-level parity checks where not already corpus-verified.

Detailed heavy tables remain split into:
- `RGM_FIELDS_FULL.md`
- `RGM_ENUMS.md`

## Tooling Script-Change Sidecar (`MAP_CHANGES`)
The Java mod-manager defines a text sidecar used to patch RGM scripts without reauthoring binary chunks directly.

Observed file model (`MapChanges.java`):
- non-indented line: map name key (for example `ISLAND`),
- indented line: `position<TAB>lineText`, where `position` is the script line index,
- positions are stored in a `TreeMap` (ascending apply order).

Observed edit encoding:
- insertions are stored as one or more `lineText` entries at a position,
- deletions are represented by a leading `"null"` sentinel entry at that position in serialized output,
- mixed replace-at-position behavior is represented as delete sentinel + insertion lines.

Apply behavior (`MapFile.getModifiedScript`):
- if no change entry exists for line `pos`, original line is emitted,
- if first change item equals `"null"`, original line is suppressed (delete),
- any non-`"null"` change lines at that position are emitted in order.

Load-order merge behavior (`RedguardModManager.applyChanges`):
- change files from enabled mods are read in mod load order,
- resulting merged edits are applied against backup RGM files,
- changed maps are rewritten via `writeMap(...)`; unchanged maps are copied from backup.

Workspace C++ parity (`RedguardsMapChanges` / `RedguardsMapFile`):
- same deletion model: literal `"null"` as first entry at a position suppresses original line,
- same apply order: original line first (unless deleted), then insertion lines in listed order,
- map-name normalization on read is stricter than Java sidecar reader:
  - forced uppercase,
  - optional `MAPS/` or `MAPS\\` prefix stripped,
  - optional `.RGM` suffix stripped.
- C++ reader now preserves inserted line text after the first tab without trimming leading/trailing
  spaces.
- C++ apply path now supports synthetic EOF positions for append-only edits.

Implementation caveats observed:
- Java-side behavior may still trim or otherwise normalize inserted script lines differently; treat
  cross-tool sidecar round trips cautiously until parity is rechecked.

## Corpus Validation Notes
- Retail GOG corpus confirms map-scoped `.RGM` files as scene/object/script hubs with co-located companion families (`.PVO/.RAN/.ANM/.MPM`).
- Multiple parser implementations (Java + Unity + workspace C++) agree on core section framing and RAHD-driven indirection model.
- Optional/conditional section behavior (`RAVC`, `RAFS`) is observed across docs and parser paths and remains important for round-trip policy.

## Implementation Status (Plugin)
- Implemented:
  - section scanning/framing and major section decode,
  - RAHD-driven script/string/variable extraction model,
  - script sidecar patch application model (`MAP_CHANGES`) in Java and workspace C++ paths,
  - retail-validated script reader/writer round-trip for sampled `RGM` files.
- Partial:
  - `RAHK`, `RAEX`, `RAGR`, `WDNM` semantic depth,
  - some runtime-specific coordinate/angle conventions,
  - remaining script edge cases not yet exercised across the full retail corpus.
- Not implemented:
  - full field-level canonical semantics for all under-modeled sections,
  - complete round-trip preservation policy across all optional/unknown blocks in every tool path.

## Open Questions / RE Backlog
1. Resolve `RAAT`/`RAHK` naming and semantic alignment (executable labels vs SOUP-derived names).
2. Promote under-modeled sections (`RAHK`, `RAEX`, `RAGR`, `WDNM`) from structural decode to field-semantic decode.
3. Validate section-order/optionality edge cases against larger retail map corpus variants.

## Confidence Register
- High confidence: section framing/order, core section sizes, RAHD pointer coupling, and major record relationships.
- Medium confidence: portions of `RAHK`/`RAGR` semantics and some runtime conversion assumptions.
- Lower confidence: unresolved naming/semantics for ambiguous subfields.

## Related Format Docs
- `ROB.md` (model archive payloads referenced by map content)
- `TEXBSI/TEXBSI.md` (texture assets used by map/object rendering)
- `SFX.md` (sound-effect container referenced by script/runtime systems)
- `RESTORATION_EVIDENCE_MATRIX.md` (project-backed unresolved-field matrix and next-step queue)

## Retail GOG Validation (Install Corpus)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/maps`

Observed:
- Retail `maps/` includes map-scoped `.RGM` files matching primary world regions.
- Co-located `.PVO/.RAN/.ANM/.MPM` families align with RGM as scene/object/script hub.
- Workspace C++ script reader/writer round-trip is now confirmed on retail samples:
  - `BELLTOWR.RGM`
  - `ISLAND.RGM`
  - `PALACE.RGM`
  - `TAVERN.RGM`
  - `CATACOMB.RGM`
- The critical parser-front-end repair was preserving top-level token structure in real script text instead of splitting on raw whitespace.

## Compatibility Documentation

Legacy `RGM_FIELDS_FULL.md` and `RGM_BEHAVIOR_NOTES.md` names are migration indexes only. Field and behavior details should be maintained in `RGM.md`, `RGM_SCRIPT_BYTECODE.md`, `RGM_TASK_FUNCTION_RUNTIME.md`, and `blocks/<TAG>.md`.

## References
- External reference documentation (2026-03-08)
- `classic_es_file_formats.md` (classification-level notes)
- `redguard-mod-manager-main/src/redguard/MapFile.java`
- `redguard-mod-manager-main/src/redguard/MapHeader.java`
- `redguard-mod-manager-main/src/redguard/MapChanges.java`
- `redguard-mod-manager-main/src/modManager/RedguardModManager.java`
- `redguard-mod-manager-main/src/modManager/ScriptEditor.java`
- `src/games/redguard/redguardsmapchanges.h`
- `src/games/redguard/redguardsmapchanges.cpp`
- `src/games/redguard/redguardsmapfile.cpp`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMapImport/RGRGMFile.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/rgmtst/rgmtst.cs`
- Retail corpus: `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/maps/*.RGM`
