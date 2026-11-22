# Redguard SAVE (SAVEGAME.SAV) — Format Overview

Purpose
-------
This document is the single, high-level entrypoint for the Redguard SAVE/TSG file system used by the project. It summarizes container layout, chunk framing, known tags, parser guidance, and where to find block-level details and corpus validation data.

Quick summary
-------------
- Container is chunk/section-based. The parser scans the file for 4-byte ASCII chunk tags and extracts bounded payloads.
- Canonical chunk framing used by the scanner: `Tag[4]` (ASCII) + `Length[4]` (big-endian u32) + `Payload[Length]`.
- Within chunk payloads, endianness and inner-field encodings are block-specific (see block docs).

Where to find more detail
-------------------------
- Full, implementation-ready spec and corpus notes: `SAV_TSG_FULL.md` (same folder).
- Per-chunk/block documentation and work-in-progress decoders are located in the `blocks/` folder alongside this file.

File / slot layout (practical)
-----------------------------
- Save slots are directory-based. Typical slot directory looks like `SAVEGAME/SAVEGAME.001/SAVEGAME.SAV`.
- Parser expects a `SAVEGAME.SAV` file inside the slot directory; some slots in the corpus lack it (treated as broken/incomplete snapshots).

Top-level framing observed in corpus
-----------------------------------
- The file typically begins with a header region anchored by a `SVGM` marker. The parser extracts a few fixed metadata fields from this region (format/version string, save title).
- The parser then scans for chunks. Observed canonical top-level chunk order in current corpus (observed across samples):
  `SVGM, THMB, SCR3, SVGF, SMGR, SMOB, SMRP, SMCO, SMSH, SVCM, SMFM, SVRG, SVGV, SVG2, SVIT, SVCB, SVMV, SVMD`

Known/common tags (short)
-------------------------
- `SVGM` — container signature / header
- `THMB` — thumbnail preview payload (observed as BE length 0x00018000 in corpus; 256x192 RGB565)
- `SVIT` — player/inventory metadata (parser extracts several fields such as gold)
- `SVMD` — location metadata (parser uses heuristics to extract `?xxx` tokens and active location id)
- `*.TSG` sidecars — auxiliary location/state hints; parser now scans all matching sidecars and uses the strongest token as a fallback location cue
- `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVRG`, ... — many tags present and currently treated as opaque payloads by the runtime parser

REDEDIT editor note:
- External VB6 editor `REDEDIT` contains the literal `SVIT` and explicit item-edit UI strings. It exposes four editable values: Gold, Health, IronSkin, and Strength, and uses 4-byte positional I/O. See `SAV_TSG_FULL.md` for full REDEDIT evidence and candidate constants.

Chunk framing and extraction (parser rules)
-----------------------------------------
1. The parser scans the file for 4-byte ASCII tags.
2. For each occurrence it reads the following 4 bytes as a big-endian `u32` length and validates `payloadOffset + length <= fileSize`.
3. Truncated/out-of-bounds occurrences are skipped.
4. The parser uses the *last bounded-valid* occurrence of a given tag for metadata extraction (helper `findLastChunkPayload`).

Endianness notes
----------------
- The chunk-length field is interpreted as big-endian (`BE u32`).
- Fields inside payloads are block-specific; most extractor heuristics in the parser use little-endian reads for intra-payload numeric fields (see individual block docs).

Parsing guidance / pseudocode
----------------------------
Simple chunk-scan pseudocode used by the scanner:

```
for pos in 0..fileSize-8:
  if isASCIIAlpha(data[pos:pos+4]):
    len_be = be_u32(data[pos+4:pos+8])
    if pos+8+len_be <= fileSize:
      recordChunk(tag=data[pos:pos+4], payloadOffset=pos+8, length=len_be)
    else:
      skip

for each tag in recordedChunks:
  use last-recorded-bounded occurrence for extraction
```

Example hex layout (conceptual)
-------------------------------
- [0x00] `53 56 47 4D` (`SVGM`) — header
- [0x08] header C-strings (version, title, etc.)
- [var]  `54 48 4D 42` (`THMB`) `00 01 80 00` (BE length) `...payload...`
- [var]  `53 56 49 54` (`SVIT`) `00 00 07 68` (BE length) `...payload...`

Where to add new block docs / decoders
-------------------------------------
- If you decode a tag or add structure information, add a block doc under `blocks/` named `TAG.md` (for example, `SMRP.md`).
- Update `SAV_TSG_FULL.md` and this README with a short summary and a link when a decoder becomes available.

Corpus & validation
--------------------
- A validated corpus and notes live in `SAV_TSG_FULL.md` and companion files referenced there (`TSG_FILES_INDEX.*`, `SAVE_PAIR_DIFFS.md`). Use those artifacts when testing new decoders.

Developer tools
---------------
- `RedguardsSaveGame::chunkTableDebugText()` (implementation) — dumps a tab-delimited chunk table useful for inspections.
- Use the chunk table dump to quickly identify chunk offsets, lengths, and out-of-bounds/truncated occurrences.

Next steps / TODO for contributors
---------------------------------
- Expand per-block docs in `blocks/` for the remaining tags, starting with variable-length gameplay-state blocks (`SMGR`, `SMOB`, `SMRP`, `SVRG`).
- Add unit/fixture saves for each decoded block for regression testing.

References
----------
- See `SAV_TSG_FULL.md` for a large, implementation-oriented spec and corpus findings.
- Per-block docs: `blocks/` (one file per tag)

# SAV/TSG Block Documentation Set

Last updated: 2026-03-11

## Purpose
This folder contains the save-system overview (`SAV_TSG.md`), full canonical monolith (`SAV_TSG_FULL.md`), and per-chunk block docs for `SAVEGAME.SAV`, in the same structure style as `RGM/`.

## Canonical Full Spec (lossless copy)
- `SAV_TSG_FULL.md`

## Split Section Docs
- `01_SCOPE_LAYOUT_AND_ORDER.md`
- `02_CORPUS_VALIDATION.md`
- `03_PARSER_BEHAVIOR.md`
- `04_STATUS_AND_BACKLOG.md`

## Block Files (observed chunk order)
1. `blocks/SVGM.md`
2. `blocks/THMB.md`
3. `blocks/SCR3.md`
4. `blocks/SVGF.md`
5. `blocks/SMGR.md`
6. `blocks/SMOB.md`
7. `blocks/SMRP.md`
8. `blocks/SMCO.md`
9. `blocks/SMSH.md`
10. `blocks/SVCM.md`
11. `blocks/SMFM.md`
12. `blocks/SVRG.md`
13. `blocks/SVGV.md`
14. `blocks/SVG2.md`
15. `blocks/SVIT.md`
16. `blocks/SVCB.md`
17. `blocks/SVMV.md`
18. `blocks/SVMD.md`

## Companion Research Artifacts
- `../TSG_FILES_INDEX.md`
- `../TSG_FILES_INDEX.csv`
- `../SAV_TAG_PROFILE_2026-03-11.md`
- `../SAVE_PAIR_DIFFS.md`

## Status and Backlog

Canonical save status and the consolidated RE backlog are maintained in `04_STATUS_AND_BACKLOG.md`.

## Migration Note
The large monolithic save spec is preserved in this folder as `SAV_TSG_FULL.md`.
Content can be moved chunk-by-chunk into `blocks/*.md` during dedicated save RE work.

Current state:
- Core monolithic content has been section-split for readability while preserving the original top-level document as compatibility source.

## Screenshot and Thumbnail Rendering

The save parser is renderer-aware:
- `THMB` supplies the 256x192 thumbnail for saves whose thumbnail path is valid.
- Glide-rendering saves can fall back to `SCR3`, decoded as a 640x480 RGB565 framebuffer.
- Software-rendering saves can use `SCRN`, decoded as a 640x480 indexed framebuffer with the active map palette.
- Screenshot decode failures are non-fatal and are not sufficient by themselves to mark a save slot broken.

The block details are in `blocks/THMB.md`, `blocks/SCR3.md`, and `blocks/SCRN.md`.
