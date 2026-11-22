# RGM Script Bytecode (RASC) Reference

Last updated: 2026-03-29

## Scope
This is a parser-derived opcode and operand reference for the RASC script stream, extracted from the Java toolchain and reconciled against the current C++ implementation plus retail `RGM` round-trip validation.

## Endianness
- Most scalar integers in script stream are little-endian.
- Address labels are represented as 32-bit little-endian offsets in stream and rendered as `#XX` labels by tooling.

## Core Value Type Opcodes
| Opcode | Meaning | Primary Operand Pattern |
|---:|---|---|
| 0 | Task | `u16 functionId`, `u8 paramCount`, then params |
| 1 | Multitask | same as Task |
| 2 | Function call | same as Task |
| 3 | If | conditional chain, conjunction bytes, end-of-block offset |
| 4 | Goto | `u32 labelOffset` (0 means bare End/Goto form) |
| 5 | End | `u32 labelOffset` (optional target) |
| 6 | Flag reference | `u16 flagIndex` + mode-dependent tail |
| 7 | Numeric | mode-dependent integer/string payload |
| 10 | Local variable | `u8 varIndex` + mode-dependent tail |
| 15 | Object.Reference++ | object selector + reference + unary tail |
| 16 | Object.Reference-- | object selector + reference + unary tail |
| 17 | Gosub | `u32 labelOffset` |
| 18 | Return | none |
| 19 | Endint | none |
| 20 | Object.Reference | object selector + reference + mode-dependent tail |
| 21 | String table reference | `u32 stringIndex` into header string table |
| 22 | Numeric (special) | same base payload class as 7, used for specific global-flag tasks in parser |
| 23 | `<Anchor>=` literal | `u8 anchorValue` |
| 25 | Object.Task (task type) | object selector + nested task encoding |
| 26 | Object.Task (function type) | object selector + nested task encoding |
| 27 | TaskPause | `u32 labelOffset` |
| 30 | `if <ScriptRv> =` branch | `u8 value`, then fixed-size block in parser |

## Condition/Operator Encoding

### Comparisons
| Byte | Text |
|---:|---|
| 0 | `=` |
| 1 | `!=` |
| 2 | `<` |
| 3 | `>` |
| 4 | `<=` |
| 5 | `>=` |

Conjunction byte after each comparison clause:
- `0` => end condition list
- `1` => `and`
- `2` => `or`

### Formula/Assignment Operators
| Byte | Text | Notes |
|---:|---|---|
| 0 | (none) | stop/no-op |
| 1 | `+` | binary |
| 2 | `-` | binary |
| 3 | `/` | binary |
| 4 | `*` | binary |
| 5 | `<<` | binary |
| 6 | `>>` | binary |
| 7 | `&` | binary |
| 8 | `|` | binary |
| 9 | `^` | binary |
| 10 | `++` | unary; followed by an extra pad byte in parser |
| 11 | `--` | unary; followed by an extra pad byte in parser |

## Object Selector Encoding
Used by object-dot forms (`20`, `25`, `26`, `15`, `16`):
- `0, padByte` => `Me.`
- `1, padByte` => `Player.`
- `2, padByte` => `Camera.`
- `4, u8 stringIndex` => name from header string table
- `10, u8 varIndex` => value from local variable table

Reference name then follows as `u16` index into SOUP refs/equates table.

## Task Encoding
Task opcodes (`0/1/2`) payload:
1. `u16 functionId`
2. `u8 paramCount` (except functionId 0 special-case in reader comments)
3. encoded parameters (each is itself a value form, read in PARAMETER mode)

## Parameter-Type Specialization (Reader)
Reader post-processes some parameter payloads by current task+parameter index:
- Dialogue-like parameters (`RTX`, `AddLog`, etc.) map numeric/string values to RTX labels and subtitle comments.
- Map parameters (`LoadWorld`) map IDs to map names.
- Item parameters map IDs to item names.

Concrete `currentTask + paramIndex` keys in Java reader:
- Dialogue: `ACTIVATE0`, `AddLog0`, `AmbientRtx0`, `menuAddItem0`, `RTX0`, `rtxAnim0`, `RTXp0`, `RTXpAnim0`, `TorchActivate0`
- Map: `LoadWorld0`
- Item: `ActiveItem9`, `AddItem0`, `DropItem0`, `HandItem0`, `HaveItem0`, `SelectItem0`, `ShowItem0`, `ShowItemNoRtx0`

Parser-side companion behavior:
- text literals are treated as dialogue payloads only when current function name is in dialogue-function set (`ACTIVATE`, `AddLog`, `AmbientRtx`, `menuAddItem`, `RTX`, `rtxAnim`, `RTXp`, `RTXpAnim`, `TorchActivate`),
- otherwise quoted literals emit as normal string-table references (`opcode 21`).

## Label Semantics
- Branch targets are absolute script-stream offsets within current header script byte array.
- Tooling renders labels as uppercase hex with `#` prefix (`#%02X`).
- Parser stores temporary unresolved labels and backfills after parsing block bodies.

Implementation caveat (writer path):
- unresolved label references are emitted as 4-byte placeholders,
- backfill writes only the first two bytes from a LE `short` offset and leaves upper two bytes as initially zero,
- this effectively constrains backfilled label values to 16-bit range in this parser implementation path.

## Script Header Coupling
RASC interpretation depends on RAHD-provided metadata:
- script length
- script data offset
- execution start (`scriptPC`)
- string count/index into RASB
- variable count/offset into RAVA

## Parser Front-End Normalization (`ScriptParser`)
Before bytecode emission, legacy Java tooling performs a text normalization pass:
- strips trailing `//` comments,
- inserts spacing before unary `++/--`,
- rewrites `<MapName>` and `<ItemName>` tokens into numeric IDs using loaded database maps.

Current workspace C++ behavior is intentionally stricter and now validated against real retail scripts:
- it does **not** strip spaces after commas globally,
- it strips `//` comments only when they occur outside quoted text,
- task/function parameter lists are split only on top-level commas, preserving commas inside quoted string/dialogue parameters,
- `if` clauses and formula expressions are tokenized at top-level whitespace only, preserving function calls like `PlayerArc(1024, 250)` and comparisons like `var2 < 30`,
- it round-trips unary object-reference suffix forms (`++` / `--`) emitted by the reader.

Token-rewrite constraint in this parser path:
- angle-bracket token rewrite still uses pattern `<[a-zA-Z0-9 ()']+>`, so only that character set is normalized through map/item ID substitution.

Emission workflow:
- parses script per header block,
- resolves labels in a second pass by backfilling previously deferred offsets,
- accumulates per-header bytecode and stores script-data offsets for RAHD rewrite.

Special-case opcode selection in parameter mode:
- opcode `22` is emitted instead of `7` for select global-flag tasks (`TestGlobalFlag`, `SetGlobalFlag`, `ResetGlobalFlag`) in this parser implementation path.

## Retail Validation Status
Retail C++ reader + writer round-trip has been validated against installed Redguard `RGM` files:
- `BELLTOWR.RGM`
- `ISLAND.RGM`
- `PALACE.RGM`
- `TAVERN.RGM`
- `CATACOMB.RGM`

This validation specifically confirmed the repaired parser front-end handles:
- function-call arguments with spaces after commas,
- top-level comparison tokenization in `if` clauses,
- nested block parsing for real shipped scripts.

## Caveat
This table now reflects a parser/writer path proven against a real retail sample set, but it is still not an exhaustive proof over every shipped `RGM` in the corpus.

## Unity Parser Cross-Check
- Unity script runtime/parser (`RGRGMScriptStore`) uses the same core opcode families (`Task`, `Function`, `If`, `Goto`, `End`, object selectors, gosub/return).
- Branch label handling in Unity is also offset-based and decoded from the same underlying script stream model.
- This provides independent confirmation that RASC is a VM-like bytecode stream with mixed expression and object-reference opcodes rather than a tokenized text script.

### Unity `io_tst/rgmtst` Opcode Coverage Notes
Observed in `Assets/Scripts/RGFileImport/RGMData/RGRGMScriptStore.cs` as used by `io_tst/rgmtst/scriptstst.cs`:

- Explicitly handled by `readInstruction(...)`:
	- `0x00` task, `0x01` multitask, `0x02` function,
	- `0x03` if, `0x04` goto, `0x05` end,
	- `0x06` flag,
	- `0x07` immediate and `0x16` immediate alias,
	- `0x0A` local variable,
	- `0x11` gosub, `0x12` return, `0x13` endint,
	- `0x14` object reference,
	- `0x15` string reference,
	- `0x19/0x1A` object task/function forms,
	- `0x1B` taskpause,
	- `0x1E` scriptrv.

- Confirmed gaps/limitations in this path:
	- no dedicated handling for opcodes `0x0F/0x10` (Object.Reference++ / Object.Reference-- forms in some references);
	- object selector supports `0x00/0x01/0x02/0x04` only (no `0x0A` local-variable selector in this reader);
	- unknown opcode throws (`NIMPL`) and aborts parse.

- Runtime-completeness caveat:
	- `ENDINT` (`0x13`), `TASKPAUSE` (`0x1B`), and `SCRIPTRV` (`0x1E`) currently log/debug with placeholder behavior rather than full semantic implementation.

### Unity Execution-Loop Notes
From `RGRGMScriptStore.ScriptData` runner behavior:
- `runScript()` initializes instruction pointer from `scriptPC` and executes until `readEnd()` returns sentinel `0xDEAD`.
- `readEnd()` (`0x05`) reads a `u32` target and updates persistent `scriptPC` before returning end sentinel.
- `readGoSub()`/`readReturn()` (`0x11`/`0x12`) use an explicit integer callstack (`push return IP` / `pop resume IP`).
- `runScript()` includes a hard loop guard (break after ~1024 instructions with debug bailout) in this implementation path.
- `tickScript()` runs exactly one instruction per tick (single-step integration mode).

## Open Questions (Non-SAV)
- Opcode `0x0F`/`0x10` semantics remain unresolved in Unity runner path (no explicit `readInstruction` cases).
- Object selector byte `0x0A` (variable-indirect object addressing in some references) is not accepted by Unity `readObject()`.
- `readObjectRef` uses `ReadUInt16() & 0xFF` for reference ID, suggesting upper-byte truncation assumptions that need corpus confirmation.
- LHS/RHS operator bytes consumed in `readFlag`/`readVar`/`readObjectRef` are currently ignored; semantic impact is unknown.
- `ENDINT`/`TASKPAUSE`/`SCRIPTRV` are decode-visible but runtime-placeholder in Unity, so control-flow parity is still incomplete.

## Targeted Validation Plan (Non-SAV)
1. Add bytecode tracer hooks around `readInstruction`, `readObject`, and `readObjectRef` to log raw opcode/selector/reference bytes before interpretation.
2. Run tracer on high-script-density maps (`ISLAND`, `PALACE`, `OBSERVE`) and capture frequency tables for unresolved opcodes/selectors.
3. Cross-compare traces with Java `ScriptReader` textual output at matching offsets to infer intended semantics for unknown branches.
4. Create synthetic script fixtures for `0x0F/0x10` and selector `0x0A` paths, then verify parser/writer round-trip and VM control-flow behavior.
5. Promote findings into canonical opcode tables only when both parser-path and runtime-path behavior agree.

## Sources
- `redguard-mod-manager-main/src/redguard/ScriptReader.java`
- `redguard-mod-manager-main/src/redguard/ScriptParser.java`
- `redguard-mod-manager-main/src/redguard/ScriptInstruction.java`
- `redguard-mod-manager-main/src/redguard/MapHeader.java`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGRGMScriptStore.cs`
