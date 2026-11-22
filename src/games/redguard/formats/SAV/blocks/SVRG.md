# SVRG

## Status
Known tag, not decoded, but now confirmed as a meaningful structural companion to `SVMD`.

## Purpose
Known active save-state chunk with meaningful size variability. Current evidence suggests it
participates in the same global-versus-area layering family as `SVMD`.

## Known Structure
- Chunk framing: `Tag[4] + be u32 length + payload`.
- The first observed `32` bytes are identical across all sampled `SAVEGAME.SAV` and `.TSG`
  payloads examined so far:
  - `00 00 00 00`
  - `02 00 00 00`
  - `00 00 00 00`
  - `02 00 00 00`
  - `00 00 00 00`
  - `03 00 00 00`
  - `00 00 00 00`
  - `00 00 00 00`
- This strongly suggests a shared header/prefix followed by a larger variable body.
- Observed payload lengths vary widely by map/save state, for example:
  - `1528`
  - `1692`
  - `1784`
  - `2176`
  - `2716`
  - `4136`
  - `5512`
  - `6244`
  - `6276`
  - `15956`

## Notes
- Prioritized for trial/diff decode.
- No direct parser handler currently exists in `RedguardsSaveGame`.
- Currently treated as opaque payload by metadata parser.
- Exact payload equality is currently used as the second tie-breaker after `SVCB` when multiple
  `.TSG` files exactly match `SAVEGAME.SAV` on `SVMD`.
- This makes `SVRG` a probable layered companion block even though its internal fields are still
  unresolved.
- Current corpus behavior suggests `SVRG` is a larger area/world-state payload, not a compact
  selector block like `SVCB`.
- In the currently tested ambiguous `SVMD` cases, `SVRG` exact equality did not resolve any case
  that `SVCB` had failed to resolve.
- So `SVRG` remains relevant, but its main value is likely in deeper state modeling rather than
  acting as a strong standalone discriminator.

## Known Fields
- None.

## Current Observations
- Stable prefix across sampled files:
  - `u32 0`
  - `u32 2`
  - `u32 0`
  - `u32 2`
  - `u32 0`
  - `u32 3`
  - `u32 0`
  - `u32 0`
- Variable-length body after the prefix is the part most likely carrying the actual per-area
  gameplay state.

## Unknowns
- Internal semantic layout unresolved.
- Whether the stable 32-byte prefix is a header, table preamble, version marker set, or fixed
  state vector.
- How much of the variable body is per-map object state versus active-runtime state.

## References
- `../SAV_TSG_FULL.md`

