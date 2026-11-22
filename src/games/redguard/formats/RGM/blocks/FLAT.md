# FLAT

## Status
Documented (usage unclear).

## Purpose
Unresolved terminal payload section in current corpus/docs.

## Layout
- Raw payload section near end of stream, after `WDNM` and before `END `.
- No section-local record model is currently established in workspace docs.

## Relationships
- Sits in the same late-file cluster as `MPSZ` and `WDNM`, which suggests render/navigation companion data rather than core script metadata.
- Name overlap with `MPSF` ("flat objects") is noted, but current evidence is insufficient to collapse them into the same semantic role.

## Writer Notes
- Raw payload preserved during rewrite copy-window stage.

## Unknowns
- Whether `FLAT` is a table, blob, or late-bound resource reference block.
- Whether its payload is consumed directly by runtime, tooling only, or specific map variants.
- Whether it is structurally coupled to `MPSF` or only shares naming terminology.
