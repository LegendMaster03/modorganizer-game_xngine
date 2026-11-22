# SVMD - SaVe Map Data

## Status
Partially modeled, with the binary record layout now confirmed but the header/trailer
selector semantics still unresolved.

## Purpose
Location/state metadata chunk used by current location display heuristics.

## Confirmed Layout
- The parser should use the last bounded-valid `SVMD` chunk in `SAVEGAME.SAV`.
- Meaningful `SVMD` payloads observed in the retail save corpus share a fixed 8-byte
  header followed by repeating 30-byte records.
- The first record begins at payload offset `0x08`.
- Each additional record begins `0x1E` bytes later.
- Within each 30-byte record:
  - `+0x00`: unknown `u64`-like field
  - `+0x08`: unknown `LE u32`
  - `+0x0C`: unknown `LE u32`
  - `+0x10`: unknown `LE u32`
  - `+0x14`: unknown `LE u32`
  - `+0x19`: 4-byte ASCII code such as `?smk`, `?isz`, `?bye`
  - `+0x1C`: unknown `LE u16` flag, observed values `0` and `1`

## Corpus Observations
- For all non-empty `SVMD` blocks currently observed in the local Redguard save corpus:
  - the first `?xxx` code starts at payload offset `0x21`
  - subsequent codes are exactly `0x1E` bytes apart
- For many saves, the `SAVEGAME.SAV` `SVMD` code set exactly matches one or more sibling
  `.TSG` files, which supports the current working model that `SVMD` participates in the
  same global-versus-area layering as other save chunks.
- Observed lengths include:
  - `0x09` (`9`) with an all-zero header and a trailing `00` byte; this appears to be an
    effectively empty `SVMD` variant seen in early/tutorial saves
  - `0x35` (`53`) with 1 record
  - `0x45` (`69`) with 2 records
  - `0x63` (`99`) with 3 records
  - `0x71` (`113`) with 3 records plus a 15-byte trailer
  - `0x81` (`129`) with 4 records
  - `0x8F` (`143`) with 4 records plus a 15-byte trailer
  - `0x9F` (`159`) with 5 records
  - `0xBD` (`189`) with 6 records
  - `0xDB` (`219`) with 7 records
  - `0xF9` (`249`) with 8 records

## Header / Trailer
- Header bytes `0x00..0x07` are present in all meaningful `SVMD` payloads.
- Their semantics are not yet confirmed.
- Two trailer families are observed:
  - a 1-byte trailing `00`
  - a 15-byte trailer after the last full 30-byte record
- The 15-byte trailer is not yet understood and may represent a partial active/current
  record or some other per-chunk state.

## Current Code State
- The C++ implementation now reads `SVMD` by direct indexed records instead of regex
  scanning the whole payload.
- The active/current selector is still not proven, so location resolution should remain
  conservative when the selector does not clearly identify one record.
- Current code therefore does not elevate arbitrary `SVMD` record codes into the normal
  user-facing location field when no selector is known.
- Exact `SAVEGAME.SAV` -> `.TSG` `SVMD` equality is now treated as a structural/internal
  identity signal only, not as a user-facing location name.
- When multiple `.TSG` files have the same `SVMD`, current code narrows the candidate set
  by:
  - exact `SVCB` equality
  - then unique `SVCB word5` match
  - then unique `SVCB word3` match
  - then exact `SVRG` equality
- In the current local save corpus, that structural stack is now sufficient to reduce every
  observed `SVMD`-ambiguous save slot to a single `.TSG` basename.
- The remaining unresolved part of location is therefore display-name proof, not structural
  `.TSG` identity.
- Additional record-level dumps from structurally resolved `ISLAND` saves (`029`, `037`,
  `038`) show that the per-record numeric fields and trailing `u16` flag are stable by
  record code, not by active player location:
  - `+0x10` is constant (`6656`) across all observed records
  - `+0x14` behaves like a slot/order field in `256`-byte steps (`0, 256, 512, ...`)
  - the trailing `u16` flag at `+0x1C` repeats consistently for the same code across saves
    (for example `?smk=363`, `?bye=357`, `?slg=103`)
  - the trailer in these common `ISLAND` cases is only a single `00` byte
- That strongly suggests `SVMD` is acting as a per-map label/state table, not as the primary
  active-location selector for those saves.
- Direct RTX inspection of the `ISLAND` code set from save `037` shows why user-facing
  location still remains blank even when structural identity is correct:
  - `?smk` -> `STROS M'KAI`
  - `?isz` -> `ISZARA`
  - `?hay` -> `HAYLE`
  - `?emp` -> `EMPIRE`
  - `?hlp` -> `HELP`
  - `?slg` -> `SOULGEM`
  - `?hsg` -> `HELP WITH SOULGEM`
  - `?bye` -> `BYE`
- So the remaining ambiguity is not "no labels exist". It is "the active/current SVMD record is
  still unknown, and the code set mixes one real place label with several story/topic labels".
- The current common developer `Area` token (`HBDOCK06`) does not behave like a final displayed
  location name, but it is not junk:
  - it appears as a real object/reference-family identifier inside `ISLAND` assets
  - neighboring identifiers such as `HBDOCK02`, `HBDOCK03`, `HBDOCK04`, `HBDOCK05`, `HBDOCK07`,
    `HBDOCK08`, and `HBDOCK10` are also present
  - this makes the `HBDOCKxx` family the strongest current lead for structurally deriving
    harbor/sub-area identity without relying on mixed subtitle tables
- User-facing names are **not** currently treated as solved:
  - structural identity comes from `SVMD` / matching `.TSG` files
  - several attempted display-name derivation paths exist as developer evidence only
  - these include:
    - current-save `SVMD` `?xxxx` code intersection against matched-map label sets
    - parsed-map/script-derived dialogue comment candidates
    - transition-derived `LoadWorld(<TARGET>)` context
  - all of those can still surface non-location subtitles such as `BYE`, `HELP`, or
    other topic/person labels on otherwise-correct maps
  - therefore the normal visible `Location` field should stay blank unless a future
    derivation path is proven structurally
- `WORLD.INI` is also used as map metadata:
  - maps with a `world_world[n]` entry are treated as outdoor/world-space maps
  - for those maps, world-style `?xxxx` RTX noun labels are preferred over local
    `ENTER ...` prompts when choosing the display name
- The parsed-script and transition-derived display experiments remain useful for RE, but are not
  yet safe enough for production display.
- If no structurally proven display name source exists, the user-facing `Location` field should
  be left empty rather than falling back to raw area tokens or arbitrary subtitles.
- This is intentionally stricter than mnemonic token guessing, because map-local RTX labels and
  script-adjacent dialogue comments can still include prompts, dialogue topics, and other
  non-title strings.

### Current hard display-name cases
- `VILE`:
  - `WORLD.INI` defines `world_map[14]=MAPS\\vile.rgm` with no corresponding `world_world`
    entry
  - unlike clear interior maps such as `PALACE` and `MGUILD`, its render/environment metadata
    is outdoor-style (`sky61.GXA`, sun/sky/fog values similar to world-space maps)
  - so current evidence treats it as an exterior-like special map identity, but not a normal
    `world_world`-bound outdoor map
  - local `.RGM` label scan is dominated by dialogue/riddle labels (`?vqa` .. `?vql`) and other
    non-title topics
  - the strongest place-like token seen so far is `?hid = HIDEOUT`
  - this is not yet strong enough to promote a final user-facing name such as
    `Clavicus Vile's Realm`
- `NECRISLE`:
  - `WORLD.INI` defines both `world_map[6]=MAPS\\necrisle.rgm` and
    `world_world[6]=MAPS\\necrisle.WLD`, so current evidence treats it as an outdoor/world-space
    map identity
  - `.RGM` contains mixed place/topic labels
  - the strongest clean place-like token seen so far is `?ncy = NECROMANCER'S TOWER`
  - this may be an interior/POI label rather than the final map title, so it remains documented
    as evidence, not yet a promoted canonical display name

## Notes
- `SVMD` is not script bytecode.
- It is a structured binary table that embeds short RTX label codes.
- See `../SAV_TSG_FULL.md`.

## Unknowns
- Exact meaning of the 8-byte header.
- Exact meaning of the four unknown numeric fields in each 30-byte record.
- Exact meaning of the `LE u16` record flag at `record+0x1C`.
- Exact meaning of the 15-byte trailer variant.
- Which record is the canonical active/current location record.
- How `SAVEGAME.SAV` `SVMD` and matching `.TSG` `SVMD` blocks are merged or overridden
  by the engine at load time.

## References
- `../SAV_TSG_FULL.md`
