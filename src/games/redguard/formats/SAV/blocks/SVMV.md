# SVMV

## Status
Known tag, not decoded.

## Purpose
Known active save-state chunk, but current corpus evidence suggests it is global/save-level
rather than an area-layer `.TSG` companion.

## Known Structure
- Chunk framing: `Tag[4] + be u32 length + payload`.
- Payload length is stable at `0x0F` (`15`) bytes in the current local corpus.
- Current observed `SAVEGAME.SAV` payload families are small flag-like byte vectors, for example:
  - `01 00 00 00 00 00 00 00 00 00 00 00 00 00 00`
  - `01 00 00 00 00 01 00 00 00 00 00 00 00 00 00`
  - `01 01 00 00 00 01 00 00 00 00 00 00 00 00 00`
  - `01 01 01 00 00 01 00 00 00 00 00 00 00 00 00`
  - `01 01 01 01 00 01 00 00 00 00 00 00 00 00 00`

## Notes
- No direct parser handler currently exists in `RedguardsSaveGame`.
- Currently treated as opaque payload by metadata parser.
- In the currently scanned local corpus:
  - `SVMV` appears in every `SAVEGAME.SAV`
  - `SVMV` appears in zero scanned `.TSG` files
- So unlike `SVMD`, `SVCB`, and `SVRG`, it is not currently a candidate layered area-state
  discriminator.

## Known Fields
- None.

## Unknowns
- Internal semantic layout unresolved.
- Meaning of the small flag-like payload bytes.
- Whether `SVMV` is purely global save metadata or participates in another non-`.TSG` state path.

## References
- `../SAV_TSG_FULL.md`
