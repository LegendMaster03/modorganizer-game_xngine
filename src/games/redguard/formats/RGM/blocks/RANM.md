# RANM

## Status
Documented, partially modeled.

## Purpose
Namespace/cutscene string payload.

## Record Model
- Null-terminated strings in a raw stream.
- Field label from full reference: `AnimationName (char[], null-terminated)`.
- Usually interpreted as raw text bytes rather than counted fixed records.

## Parser Notes
- Interpreted through RAHD pointers (`RANMOffset`, `RANMLength`).
- `io_tst` commonly reads `RANMLength - 1` chars (trailing-null convention).
- External behavior notes describe typical one-record linkage per object and include the terminator in RAHD `RANM_Length`.
- Workspace parser currently preserves payload rather than modeling the field semantically.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Relationships
- Closely coupled to `RAAN`/`RAGR` animation metadata in RAHD.
- Likely acts as naming/namespace metadata for animation groups or runtime state selection rather than general script strings.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`

## Unknowns
- Whether multi-string payloads are valid or if practical retail use is one null-terminated name per header.
- Whether this names animation sets, namespaces, cutscene banks, or another runtime concept entirely.
