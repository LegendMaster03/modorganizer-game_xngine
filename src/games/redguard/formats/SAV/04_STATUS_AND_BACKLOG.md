# SAV/TSG Status, Confidence, and Backlog

Last updated: 2026-03-29

## What Is NOT Yet Fully Parsed
- No complete decode of all known chunk families (`SCR3`, `SVGF`, `SMGR`, etc.).
- No formal decoded model for full inventory/equipment, quest/script progression, or full world/object simulation state.

Current parser characterization:
- High-value metadata parser, not full save-state deserializer.

## Discrepancies / Competing Theories
- Theory A: `THMB` post-tag field is `BE u32` payload length with fixed RGB565 payload (`256x192`).
- Theory B: `THMB` post-tag field is dimensions (`LE u16/u16`) with planar RGB payload.
- Current evidence supports Theory A for local corpus.
- Current recommendation: keep Theory B as defensive fallback until a real counterexample appears.

## Implementation Status (Plugin)
Implemented:
- signature/version/title extraction (`SVGM` + header-region fields)
- thumbnail extraction (`THMB`) with guarded dual decode path
- fixed-stride `SVIT` extraction from `SAVEGAME.SAV`
- structural location identity using `SVMD`
- `.TSG` narrowing via `SVCB`, `SVCB word5`, `SVCB word3`, and `SVRG`
- slot + auxiliary file enumeration and chunk-table dump utility

Partial:
- `SVMD` structural semantics
- user-facing location display proof
- actual player health/state fields are still not decoded from the save chunks

Not implemented:
- full decode for known opaque tags (`SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVCB`, `SVMV`)
- full gameplay-state reconstruction

## Confidence Register
High confidence:
- signature/header extraction offsets (`SVGM`, version/title)
- current-corpus chunk order and bounded framing
- `THMB` model in current corpus (`0x00018000`, RGB565 `256x192`)

Medium confidence:
- `SAVEGAME.SAV` `SVIT` item-table layout
- structural `.TSG` identity for location in the local corpus

Low confidence:
- exact `SVMD` active-record selector semantics
- any current subtitle-derived user-facing location proof

## Open Questions / RE Backlog
1. Prove a structurally safe user-facing location source.
   - current subtitle-derived evidence paths (`SVMD` labels, parsed map labels, transition
     comments) can still produce false positives like `BYE`
   - likely next lead is the `Area` token family such as `HBDOCK06`, now known to correspond to
     real `ISLAND` object/reference identifiers (`HBDOCK02`, `HBDOCK03`, `HBDOCK04`, `HBDOCK05`,
     `HBDOCK06`, `HBDOCK07`, `HBDOCK08`, `HBDOCK10`)
2. Decode `SVMD` header/trailer and active-record selector semantics.
3. Prove how `Area` tokens map to real map-object / sub-area identity.
4. Add dedicated decoders for non-implemented tags in priority order after corpus profiling.
5. Validate whether non-`0x18000` THMB variants exist in broader save corpora.

## Consolidated Save RE Queue

Evidence standards:
- Every proposed field semantic needs reproducible cross-save correlation, a controlled single-action trial, or an implementation/runtime anchor.
- Label conclusions as high, medium, or low confidence and distinguish facts from hypotheses.

Priority experiments:
1. Acquire and normalize a broader early/mid/late save corpus with slot ID, source, timestamp, and scenario.
2. Build a save ladder with gold, potion, location, and combat changes; generate per-step binary diffs.
3. Map changed ranges to known tag windows before promoting field hypotheses.
4. Test `THMB`/`SCR3`/`SCRN` decode and perturbation boundaries, including software-renderer fallbacks.
5. Validate runtime behavior for `TSG`-only slots and define safe handling for missing `SAVEGAME.SAV`.
6. Define strict rejection/warning rules for truncated or malformed chunks and user-visible diagnostics.

A save implementation is sufficiently ready for the active 1.0 workflow when critical tags have documented safe handling, missing-SAV behavior is validated or explicitly rejected, thumbnail failures are non-fatal, and unsupported save states are clearly diagnosed.

## References
- `src/games/redguard/redguardsavegame.h`
- `src/games/redguard/redguardsavegame.cpp`
- `src/games/redguard/gameredguard.cpp`
- `src/games/redguard/redguardsrtxdatabase.h`
- `src/games/redguard/redguardsrtxdatabase.cpp`

## Source
- Extracted from `SAV_TSG_FULL.md`.
