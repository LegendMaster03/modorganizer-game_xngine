# RASC

## Status
Documented + implemented for current plugin workflow, with retail round-trip validation on sampled maps.

## Purpose
Script bytecode container.

## Layout
- Raw byte payload interpreted via RAHD pointers (`RASCOffset`, `RASCLength`, `RASCStartAt`).

## Core Opcode Families
- Control flow: `If (3)`, `Goto (4)`, `End (5)`, `Gosub (17)`, `Return (18)`.
- Task/function dispatch: `Task (0)`, `Multitask (1)`, `Function (2)`, object-dot forms `25/26`.
- Data refs: `Flag (6)`, `Numeric (7/22)`, `LocalVar (10)`, `StringRef (21)`.
- Object refs: `Object.Reference (20)`, unary variants `15/16` in some tool paths.

## Selector / Operator Notes
- Object selectors used in parser paths include `Me`, `Player`, `Camera`, string-table object names, and variable-indirect forms (coverage varies by implementation).
- Condition compares use byte codes for `=, !=, <, >, <=, >=`.
- Formula operators include arithmetic/bitwise and unary `++/--` forms.

## Runtime Execution Notes
- VM starts at `RAHD.RASCStartAt` and advances by instruction decode.
- `End` handling updates persistent script PC and exits current run.
- `Gosub`/`Return` use explicit callstack behavior.
- Unity runtime path includes loop-guard bailout for runaway scripts.

## Writer Notes
- Java path sets section length to `scriptDataOffset + totalScriptLength`.
- Leading zero bytes (`scriptDataOffset`) are emitted before script blocks.
- Workspace C++ writer is now validated against real shipped script text for sampled retail maps.
- The important parser-side rule is to preserve function-call/grouping structure while tokenizing script text:
  - split task parameters on top-level commas,
  - split `if` / formula text on top-level whitespace only,
  - do not globally strip comma spacing.

## Retail Validation
Confirmed C++ reader -> text -> writer round-trip on:
- `BELLTOWR.RGM`
- `ISLAND.RGM`
- `PALACE.RGM`
- `TAVERN.RGM`
- `CATACOMB.RGM`

## Related Docs
- `../RGM_SCRIPT_BYTECODE.md`
- `../RGM_TASK_FUNCTION_RUNTIME.md`
