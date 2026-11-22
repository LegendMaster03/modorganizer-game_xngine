# SMFM

## Status
Known tag, not decoded.

## Purpose
Known active save-state chunk.

## Known Structure
- Chunk framing: `Tag[4] + be u32 length + payload`.

## Notes
- No direct parser handler currently exists in `RedguardsSaveGame`.
- Currently treated as opaque payload by metadata parser.

## Known Fields
- None.

## Unknowns
- Internal semantic layout unresolved.

## References
- `../SAV_TSG_FULL.md`

