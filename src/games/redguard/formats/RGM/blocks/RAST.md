# RAST

## Status
Documented + partially implemented.

## Purpose
Null-terminated script-string pool.

## Layout
- Concatenated C-strings (`\0` terminated).
- Offsets are section-relative and consumed through `RASB`.
- Strings are usually plain script/object/dialogue helper text, not bytecode.

## Relationships
- Referenced by `RASB` offset entries.
- A single RAHD record can reference multiple RAST strings by index.
- String payloads can be reused across RAHD records when multiple `RASB` entries point to the same RAST offset.
- `RASC` opcode `21` resolves into a per-header string index, which then resolves through the RAHD-controlled `RASB` window into this pool.
- Object selector form `4` also uses header string indices backed by `RAST`/`RASB`.

## Writer Notes
- Java path de-duplicates globally across headers (`LinkedHashSet` insertion order).
- Workspace C++ writer mirrors that global de-duplication model when rebuilding `RAST`.
- Reused strings are intentionally serialized once and referenced many times through repeated `RASB` offsets.

## Notes
- Missing terminating `\0` should be treated as malformed input; current parser paths generally clamp to section end if a null is not found.
- There is no independent count field in `RAST`; effective string counts are header-derived through `RAHD` + `RASB`.
