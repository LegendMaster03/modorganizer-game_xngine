# SWG / SWX (Redguard Input Profile Blobs)

Last updated: 2026-03-08

## Scope
Documents observed on-disk properties of `REDGUARD.SWG` and `redguard.swx` from retail install.

## Retail Corpus Observations
Validated files:
- `Redguard/REDGUARD.SWG`
- `Redguard/redguard.swx`

Observed properties:
- Both files exist in the Redguard root.
- Both have identical size in sampled install: `18610` bytes.
- Leading bytes are highly similar:
  - `SWG`: `B2 48 00 00 03 00 00 00 00 00 00 00 53 69 64 65 57 69 6E 64 65 72 ...`
  - `SWX`: `B2 48 00 00 03 00 00 00 00 00 00 00 53 69 64 65 57 69 6E 64 65 72 ...`
- Embedded ASCII string appears near header: `SideWinder`.

## Interpretation Status
- The pair looks like related profile/config binary blobs (likely input/device related), but full field semantics are not yet decoded.
- Presence of `SideWinder` strongly suggests joystick/input mapping metadata.

## Structural Clues (Current)
- Early bytes suggest fixed-header + string-table style layout rather than compressed blob framing.
- Identical file size and highly similar prefixes indicate sibling formats or mirrored stores with small mutable regions.
- Candidate mutable regions likely hold per-device mapping/profile state.

## Current Confidence
- High: file presence, size parity, header-level similarity, embedded string evidence.
- Low: exact schema and runtime ownership within engine.

## Next Reverse-Engineering Targets
1. Binary diff `SWG` vs `SWX` to isolate changing offsets.
2. Correlate byte deltas with in-game input setting changes.
3. Search executable references (`REDGUARD.EXE` / launcher tools) for `SWG`/`SWX` strings.
4. Build preliminary field map (header candidates, string table candidates, record span candidates) before semantic naming.

## Proposed Decode Workflow
- Step 1: generate stable annotated hex map for both files.
- Step 2: run controlled mutation experiments (one setting change per save) and compute offset deltas.
- Step 3: classify offsets as scalar/bitfield/string/ref-table.
- Step 4: validate by replaying settings and checking round-trip reproducibility.
