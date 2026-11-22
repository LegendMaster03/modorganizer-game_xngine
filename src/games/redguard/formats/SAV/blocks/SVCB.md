# SVCB

## Status
Known tag, not decoded, but now confirmed as a meaningful structural companion to `SVMD`.

## Purpose
Known active save-state chunk. Current evidence suggests it participates in the same
global-versus-area layering family as `SVMD`.

## Known Structure
- Chunk framing: `Tag[4] + be u32 length + payload`.
- Retail/local corpus payload length is stable at `0x20` (`32`) bytes.
- The payload is plausibly modeled as eight consecutive little-endian `u32` fields:
  - `word0` at `+0x00`
  - `word1` at `+0x04`
  - `word2` at `+0x08`
  - `word3` at `+0x0C`
  - `word4` at `+0x10`
  - `word5` at `+0x14`
  - `word6` at `+0x18`
  - `word7` at `+0x1C`

### Observed payload shape
- Most `.TSG` samples fit this sentinel-heavy pattern:
  - `word0 = 0xFFFFFFFF`
  - `word1 = 0xFFFFFFFF`
  - `word2 = 0x00000000`
  - `word3 = variable`
  - `word4 = 0x00000000`
  - `word5 = 0x00000000` or `0x00000001`
  - `word6 = 0xFFFFFFFF`
  - `word7 = 0x00000000`
- `SAVEGAME.SAV` can differ materially from the sibling `.TSG` shape, for example in
  `SAVEGAME.038`:
  - `word0 = 0x00000002`
  - `word1 = 0xFFFFFFFF`
  - `word2 = 0x00000000`
  - `word3 = 0xFFFFF998`
  - `word4 = 0x00000001`
  - `word5 = 0x00000000`
  - `word6 = 0x00000002`
  - `word7 = 0x00000001`

### Per-map stability
- Across the local corpus, `word3` and `word5` are often stable per `.TSG` basename.
- Examples:
  - `JFFERS`, `MGUILD`, `OBSERVE`, `ROLLOS`, `SMDEN`, `TEMPLE`, `VILE`
    - `word3 = 50`
    - `word5 = 0`
  - `TAVERN`
    - `word3 = 0xFFFFFEA0`
    - `word5 = 1`
  - `EXTPALAC`
    - `word3 = 0xFFFFFBFA`
    - `word5 = 1`
  - `NECRISLE`
    - `word3` observed in a small two-value family
    - `word5` observed as `0` or `1`
  - `ISLAND`
    - `word3` varies widely across progression
    - `word5` observed as `0` or `1`
- This suggests:
  - some maps have near-constant `SVCB` signatures
  - hub/outdoor maps like `ISLAND` likely carry more evolving state in the same fields

## Notes
- No direct parser handler currently exists in `RedguardsSaveGame`.
- Currently treated as opaque payload by metadata parser.
- Although field semantics are unresolved, exact payload equality is already used as the first
  tie-breaker when multiple `.TSG` files exactly match `SAVEGAME.SAV` on `SVMD`.
- This is currently the strongest evidence after `SVMD` itself for narrowing which `.TSG`
  file corresponds to the active area/state layer.
- The block is clearly structured and not random padding; the stable `32`-byte size and
  recurring sentinel pattern make it a good next reverse-engineering target.
- `word3` is the strongest candidate for a meaningful per-map/per-state field, because it
  varies widely across `.TSG` files while much of the rest of the payload stays fixed.
- `word5` is currently the strongest candidate for a small boolean/state flag (`0` or `1` in
  observed `.TSG` samples).

### Observed disambiguation strength
- Exact `SVCB` equality does help in real ambiguous save slots, but only in a minority of cases.
- In the current local corpus, among slots where multiple `.TSG` files already matched on exact
  `SVMD`, `SVCB` exact equality narrowed the result cleanly in examples such as:
  - `SAVEGAME.003` (`ISLAND` vs `JFFERS`)
  - `SAVEGAME.034` (`EXTPALAC` vs `PALACE`)
- But many other ambiguous slots still remained unresolved after exact `SVCB`, for example:
  - `SAVEGAME.007`
  - `SAVEGAME.019`
  - `SAVEGAME.029`
  - `SAVEGAME.033`
  - `SAVEGAME.038`
- So exact `SVCB` is useful, but not sufficient by itself to solve location.
- Exact equality is not the whole story, because field-level comparison also shows promise:
  - in some ambiguous slots, the save's `word5` matches one candidate map but not the other
  - example: `SAVEGAME.033` has ambiguous `SVMD` (`ISLAND` vs `TAVERN`), but save `word5=1`
    matches `TAVERN` and not `ISLAND`
- Current runtime code now uses this conservatively:
  - exact `SVCB` equality first
  - then unique `word5` match if it reduces the candidate set to exactly one
  - then unique `word3` match if it reduces the candidate set to exactly one
- This keeps field-level `SVCB` use structural rather than heuristic, because ambiguous
  field matches are not promoted.
- In the current local corpus, adding `word5`, then `word3`, and finally exact `SVRG`
  is sufficient to eliminate all remaining structural `.TSG` ambiguity after exact `SVMD`.

## Known Fields
- No semantic fields are confirmed yet.
- Current structural candidates:
  - `word3` (`+0x0C`) = strong candidate for a map/state discriminator
  - `word5` (`+0x14`) = strong candidate for a small state flag

## Unknowns
- Internal semantic layout unresolved.
- Whether `SVCB` is an override block, a companion block, or another layer selector field family.
- Whether the fields should be interpreted as signed values, unsigned values, ids, or
  coordinates/state counters.
- Why `SAVEGAME.SAV` sometimes carries a materially different sentinel pattern from the
  corresponding `.TSG` files.
- Whether `word3` / `word5` can be safely promoted from exact-match helpers to field-level
  structural tie-breaks in the location parser.

## References
- `../SAV_TSG_FULL.md`

