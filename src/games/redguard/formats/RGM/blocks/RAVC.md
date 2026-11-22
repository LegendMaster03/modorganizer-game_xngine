# RAVC

## Status
Documented, optional; workspace writer policy implemented.

## Purpose
Collision-sphere metadata.

## Record Model
- Record size: `9` bytes.
- Field layout: `s8 offsetX`, `s8 offsetY`, `s8 offsetZ`, `s16 vertex`, `s32 radius`.
- Unity derives count as `sectionLength / 9`.
- RAHD coupling caveat from external notes: `RAVC` pointer is offset-only (`RAHD.RAVC_Offset`) with no explicit per-header `length`/`count` fields.

## Corpus Notes
- External notes report usage in `CATACOMB.RGM` and `DRINT.RGM`.
- External behavior notes additionally describe dragon/golem-related usage patterns.
- Section can be entirely absent (not just empty) in many maps.

## Writer Notes
- Workspace C++ writer preserves presence/absence: emits `RAVC` only when source map contains `RAVC`.
- Legacy Java mod-manager writer still omits `RAVC` in emit sequence even when source contains it.

## Open Question
- Whether legacy Java writer paths should be upgraded to match presence-preserving `RAVC` round-trip behavior.

## Related Docs
- `../RGM_BEHAVIOR_NOTES.md`
