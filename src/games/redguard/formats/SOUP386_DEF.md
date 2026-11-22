# SOUP386.DEF (Script/Function Definition Text)

Last updated: 2026-03-08

## Scope
Documents concrete observed properties of `soup386/SOUP386.DEF` in retail install.

## Retail Corpus Observations
Validated file:
- `Redguard/soup386/SOUP386.DEF`

Observed properties:
- File size in sampled install: `43347` bytes.
- File is plain-text/ASCII-like (not opaque binary).
- Start-of-file bytes:
  - Hex: `3B 20 52 65 64 47 75 61 72 64 20 47 6C 6F 62 61 6C 20 66 75 6E 63 74 69 6F 6E ...`
  - ASCII: `; RedGuard Global function, flag...`

## Interpretation Status
- This is a definition/listing artifact for SOUP script/task/function symbols used by Redguard tooling/runtime pipelines.
- It aligns with independently observed SOUP function catalogs in Unity/io_tst (`SOUPDEF_TASKS.cs`) and runtime dispatch maps.

## Java Toolchain Parse Model (`redguard-mod-manager`)
`MapDatabase.readSoupFile()` treats SOUP as sectioned text and extracts:
- function declarations from `[functions]` until `[refs]`
- equate/reference names from `[equates]` until `auto`
- attribute names from `endauto` block lines that are not assignments
- flags from `[flags]` to EOF (with optional semicolon comments)

Line-level structures used by parser classes:
- `SoupFunction`: `type name ... paramCount` (param count parsed from token index `3`)
- `SoupFlag`: `type name value` with optional `;comment` suffix

Practical implication:
- this toolchain treats SOUP as an authoritative symbol catalog for script ID/name lookup,
- but does not interpret all textual grammar beyond these extracted token classes.

Related script-bytecode coupling observed in Java parser/reader:
- Flag references are encoded as opcode `6` + `u16` flag index into SOUP-derived `flags` list.
- Flag parameters include an extra `u16` padding value (`0`) in parser output and reader consumption.
- Global-flag task families (`TestGlobalFlag`, `SetGlobalFlag`, `ResetGlobalFlag`) use numeric opcode `22` for parameter values (instead of normal numeric opcode `7`).

Practical implication:
- SOUP catalog ordering directly affects script decompile/recompile stability for function and flag name resolution.

## Likely Content Classes
Based on header text and cross-doc behavior, expected line classes include:
- global function declarations,
- flag/attribute symbols,
- task/function IDs and possibly signatures.

These classes are not yet normalized into a machine-readable index in this repo.

## Current Confidence
- High: text nature and SOUP/global-function thematic content.
- Medium: exact parser/loader ownership in original retail runtime not yet pinned.

## Next Reverse-Engineering Targets
1. Parse line grammar into structured entries (ID/name/type/params if present).
2. Cross-link against `RGM/RGM_TASK_FUNCTION_RUNTIME.md` symbol IDs.
3. Detect mismatches between DEF catalog and in-engine implemented handlers.

## Practical Output Goal
Create a generated artifact (CSV/Markdown table) containing:
- symbol name,
- category (task/function/flag/other),
- parsed numeric ID (if present),
- parameter-arity hints,
- cross-link status to runtime handler coverage.

## Sources
- `redguard-mod-manager-main/src/redguard/MapDatabase.java`
- `redguard-mod-manager-main/src/redguard/SoupFunction.java`
- `redguard-mod-manager-main/src/redguard/SoupFlag.java`
