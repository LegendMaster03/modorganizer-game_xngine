# Documentation Gaps Backlog (Redguard Formats)

Last updated: 2026-03-11

## Purpose
Concrete list of where documentation is still weak/incomplete after consolidation.
Ordered by impact on parser correctness and sprint risk.

## 1.0 MVP Documentation Gate
Aligned to project 1.0 objective (known/active format support, stable lifecycle, clear diagnostics, no maximum-archaeology requirement).

Required before Redguard documentation can be considered 1.0-ready:
1. Save-system critical items (`Priority 0`) are resolved to operational confidence.
2. `RGM` and `TEXBSI` remaining release-blocking semantics are reduced to safe parser/writer policy + clear caveats.
3. Unsupported or uncertain cases are explicitly documented with defensive handling expectations.

Explicitly not required for 1.0:
- full semantic decode of every unknown field in every non-save format,
- deep binary archaeology for low-impact ancillary stacks (`SWG/SWX`, Miles internals),
- exhaustive historical edge-case reconstruction outside known/active mod workflows.

## Priority 0 (Save-System Critical)

1. **TSG binary schema is still mostly unknown**
- File: `SAV_TSG.md`
- Gap: many SAV/TSG tags are listed but not decoded (`SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVCB`, `SVMV`).
- Why it matters: slot-level correctness now depends on TSG-heavy state.

2. **THMB variant coverage remains unverified beyond current corpus**
- File: `SAV_TSG.md`
- Gap: current local corpus consistently uses `BE length = 0x00018000`; need confirmation that no alternate THMB encodings exist in broader saves.
- Why it matters: keeps thumbnail parser fallback policy evidence-driven.

3. **No-SAV slot loadability still unverified**
- Files: `SAV_TSG.md`, `TSG_FILES_INDEX.md`
- Gap: `.001/.009/.025` likely broken snapshots, but runtime load test not recorded.
- Why it matters: determines whether missing-SAV slots can be ignored in 1.0 rules.

## Priority 1 (Core Format Semantics)

4. **RGM semantic unknowns in critical sections**
- Files: `RGM/blocks/*.md`, `RGM/RGM.md`
- Gap: many `Unknown_*` fields in RAHD/RAEX/MPOB/MPRP/MPSL etc. remain unlabeled.
- Why it matters: safe write/round-trip and future tool behavior.

5. **RGM script runtime mapping incomplete**
- Files: `RGM/RGM_SCRIPT_BYTECODE.md`, `RGM/RGM_TASK_FUNCTION_RUNTIME.md`
- Gap: parts still marked importer-specific/partial (`MoveMarker` TODO, unresolved edge behaviors).
- Why it matters: script tooling reliability.

## 1.0 Required Set (From Priorities Above)
- `1` TSG binary schema decode to operational level for active save flows.
- `2` THMB variant confidence bounds and fallback policy evidence.
- `3` No-SAV slot loadability policy validation (or explicit safe rejection policy evidence).
- `4` RGM critical unknowns that affect writer/runtime safety.
- `5` RGM runtime mapping gaps that impact script-tool reliability.
- `7` TEXBSI BHDR unknown semantics needed for render/animation correctness.

## Post-1.0 (Keep Tracked, Not Release-Blocking)
- `6` FNT unknown-field deep semantics.
- `8` SFX per-effect byte-order certainty hardening.
- `9` GXA BBMP deeper internals.
- `10` PVO/LBM record-level deep semantics.
- `11` SWG/SWX and Miles full internals.

## Priority 2 (High-Value Format Completeness)

6. **FNT unknown field semantics**
- Files: `FNT.md`, `FNT_FIELDS_FULL.md`
- Gap: `FNHD unknown*` + `RDAT unknown*` purpose not resolved.
- Why it matters: robust editor/writer confidence.

7. **TEXBSI BHDR unknown field semantics**
- Files: `TEXBSI/TEXBSI.md`, `TEXBSI/blocks/BHDR.md`, `TEXBSI_FIELDS_FULL.md`
- Gap: multiple `Unknown*` fields and animation metadata semantics unresolved.
- Why it matters: accurate texture animation/render reconstruction.

8. **SFX per-effect endianness certainty**
- File: `SFX.md`
- Gap: section-size BE is clear, but effect-field byte-order should be byte-level confirmed.
- Why it matters: decoder correctness and export tooling.

9. **GXA BBMP internals still partial**
- File: `GXA.md`
- Gap: top-level framing is now validated, but BBMP per-image record field semantics remain partial; `GUI.GXA` BBMP length anomaly needs parser-policy hardening.
- Why it matters: complete image pipeline coverage.

## Priority 3 (Lower Immediate Risk)

10. **PVO/LBM still need deep field semantics**
- Files: `PVO.md`, `LBM.md`
- Gap: container/chunk layouts are now documented, but record-level semantics remain unresolved.
- Why it matters: lower immediate risk unless active mods rely on these heavily.

11. **SWG/SWX and Miles stack internals incomplete**
- Files: `SWG_SWX.md`, `SOUND_MILES_STACK.md`
- Gap: high-level understanding present, binary internals largely unresolved.
- Why it matters: mostly ancillary to current parser sprint.

## Suggested Next Documentation Pass Order
1. `SAV_TSG.md` (TSG tag decoding + no-SAV slot runtime verification notes)
2. `RGM/RGM.md` + `RGM/RGM_FIELDS_FULL.md` (highest-impact unknowns)
3. `TEXBSI/TEXBSI.md` / `FNT.md` unknown-field semantics
4. `SFX.md` byte-order confirmation note
5. `GXA.md` BBMP record-structure completion
6. `PVO.md` / `LBM.md` record-level semantics pass

## Recently Closed
- `RGM` optional `RAVC` writer policy in workspace plugin path:
	- implemented presence-preserving emit policy (`RAVC` is written only if present in source record set),
	- documented in `RGM/RGM.md` and `RGM/blocks/RAVC.md`.

## Non-Save Documentation Done Criteria

For active 1.0 workflows, a non-save format is done enough when:
- a canonical format page exists and is current;
- chunk grammar is sufficient for read-only parsing;
- endianness, optional chunks, sparse shards, and other runtime-sensitive caveats are explicit;
- major unresolved semantics are low-risk or linked here; and
- local Markdown links resolve.

This requires practical interoperability, not maximal archaeology of every residual field. Save-system work must still meet the separate `Priority 0` gate above.

## Canonical Save Reference

Use `SAV/SAV_TSG.md` and `SAV/blocks/*.md` for save-system scope and block details.
