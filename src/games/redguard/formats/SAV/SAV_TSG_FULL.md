# SAV/TSG File Format (Redguard)

Last updated: 2026-03-28

## Scope
This file is a cumulative technical spec for Redguard `SAV/TSG` structures.
It is intended to be detailed and implementation-ready, not a high-level overview.

Save-system handoff/navigation hub:
- `src/games/redguard/formats/SAV/README.md`

Folderized split-navigation (recommended for day-to-day reading):
- `src/games/redguard/formats/SAV/SAV_TSG.md`
- `src/games/redguard/formats/SAV/01_SCOPE_LAYOUT_AND_ORDER.md`
- `src/games/redguard/formats/SAV/02_CORPUS_VALIDATION.md`
- `src/games/redguard/formats/SAV/03_PARSER_BEHAVIOR.md`
- `src/games/redguard/formats/SAV/04_STATUS_AND_BACKLOG.md`

## Canonical Entry Point

The folder overview is `SAV_TSG.md`; split block and status details are in `blocks/` and `04_STATUS_AND_BACKLOG.md`.

## Container / File Layout
- Save slots are directory-based.
- Slot directory regex: `(?i)^SAVEGAME\.(\d+)$`.
- Valid slot requires `SAVEGAME.SAV` inside slot directory.
- Typical structure:
  - `SAVEGAME/SAVEGAME.001/SAVEGAME.SAV`
  - `SAVEGAME/SAVEGAME.001/*.TSG` and other auxiliary files
- Parser also tracks auxiliary files in the slot folder (all files except `SAVEGAME.SAV`).

Important corpus correction (from extended MO2 set):
- Some slot directories contain many `*.TSG` files and `LOGBOOK.TXT` but no `SAVEGAME.SAV`.
- Current working hypothesis: these slots are broken/incomplete snapshots due to missing `SAVEGAME.SAV`.
- Adjacent-slot evidence supports this:
  - `.001` (no SAV) vs `.002` (has SAV): only 1 overlapping TSG.
  - `.009` (no SAV) vs `.010` (has SAV): 15 overlapping TSG, 12 content-different.
  - `.025` (no SAV) vs `.026` (has SAV): 22 overlapping TSG, 11 content-different.
- Loadability of these no-SAV slots is still unverified and should be tested explicitly.

Top-level `SAVEGAME.SAV` framing used by current parser:
- `Tag[4]` (ASCII marker)
- `Length[4]` (BE `u32` in scanner path)
- `Payload[Length]`

Terminator behavior:
- No single file-level `END ` chunk is currently documented for `SAVEGAME.SAV`; parser behavior is chunk-scan based and bounded by file size.

## Section / Block Order
Observed canonical full order in current local corpus (`37` saves):
- `SVGM > THMB > SCR3 > SVGF > SMGR > SMOB > SMRP > SMCO > SMSH > SVCM > SMFM > SVRG > SVGV > SVG2 > SVIT > SVCB > SVMV > SVMD`

Known tag family observed in corpus (single occurrence each in scanned saves):
- `SVGM THMB SCR3 SVGF SMGR SMOB SMRP SMCO SMSH SVCM SMFM SVRG SVGV SVG2 SVIT SVCB SVMV SVMD`

## Common Header (SAVEGAME.SAV pre-chunk)
| Offset | Type | Name | Description |
|---|---|---|---|
| `0x00` | `char[4]` | `Signature` | Expected literal `SVGM` |
| `0x08` | `cstr[<=8]` | `FormatVersion` | Parser-extracted version string |
| `0x14` | `cstr[<=128]` | `SaveTitle` | Parser-extracted save display title |

## Corpus Validation Notes

### Retail GOG Validation (Installed Save Snapshot)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/SAVEGAME/SAVEGAME.001/SAVEGAME.SAV`

Observed marker offsets in sampled retail save:
- File size: `1,678,028` bytes
- Header bytes at `0x00000000`: `53 56 47 4D 00 00 01 10 31 2E 33 31 ...`
  - ASCII: `SVGM`
- First seen chunk tags:
  - `SVGM` at `0x00000000`
  - `THMB` at `0x00000118`
  - `SCR3` at `0x00018120`
  - `SVCM` at `0x00192776`
  - `SVMV` at `0x00199A0A`
  - `SVIT` at `0x00199272`
  - `SVMD` at `0x00199A21`

Practical confirmation:
- Retail save sample matches key tag expectations already used by parser/spec.
- Canonical thumbnail location expectation (`THMB` near `0x118`) is validated on this install sample.

### Extended Save Corpus Validation (MO2 Profile)
Validated against:
- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard`

Observed slot/file structure:
- Top-level contains legacy-style logs and save roots (`SAVEGAME/`, `SAVEGAME.001/`, `SAVEGAME.002/`).
- Nested slot directories under `SAVEGAME/` currently span `SAVEGAME.001` through `SAVEGAME.038`.
- Slot directories present: `38`; slots with `SAVEGAME.SAV`: `35`.
- Empty/missing-save slot directories in current snapshot: `SAVEGAME.001`, `SAVEGAME.009`, `SAVEGAME.025`.
- At least some slots include sidecar files (for example `ISLAND.TSG`; one sampled slot also included a helper script file).

Slot classification (current corpus):
- `TSG + SAVEGAME.SAV`: `35` slots.
- `TSG-only directory` (no `SAVEGAME.SAV`): `3` slots (`.001`, `.009`, `.025`).
- All 38 slots contain `LOGBOOK.TXT`; `TSG` counts vary widely (`1..26` files), including the `TSG-only` directories.

Adjacent-slot comparison (supports "broken then fixed" hypothesis):
- `.001 -> .002`: `.001` has 18 TSG, `.002` has 1; shared file hashes match for the overlap.
- `.009 -> .010`: `.009` has 26 TSG, `.010` has 15; `.010` is a subset by filename; mixed hash matches (3 same / 12 different).
- `.025 -> .026`: `.025` has 26 TSG, `.026` has 22; `.026` is a subset by filename; mixed hash matches (11 same / 11 different).
- In all three pairs, first slot has no `SAVEGAME.SAV`, second slot has `SAVEGAME.SAV`.

Non-`SAVEGAME.SAV` diff validation:
- Pairwise full-file diffs (excluding `SAVEGAME.SAV`) confirm additional changes beyond the metadata file:
  - differing `TSG` membership (`OnlyInA` files in each pair),
  - content changes in overlapping `TSG` files (including same-size/hash-different cases),
  - `LOGBOOK.TXT` size/content changes.
- Detailed diff artifact:
  - `src/games/redguard/formats/SAVE_PAIR_DIFFS.md`

Full `TSG` inventory log:
- Complete corpus log is maintained in:
  - `src/games/redguard/formats/TSG_FILES_INDEX.csv`
  - `src/games/redguard/formats/TSG_FILES_INDEX.md`
- Current snapshot: `750` logged `TSG` entries, `26` distinct `TSG` names, `38` slot directories represented.

Observed `SAVEGAME.SAV` corpus in current local paths:
- 37 save files scanned.
- File sizes range from `784,127` to `1,690,965` bytes.

Corpus-state caveat:
- These sampled saves are broadly similar in progression/state.
- Expected differences are limited (for example: gold value, interior vs exterior cell/location state), so this corpus should be treated as **narrow-state validation**, not full-lifecycle coverage.

Current-corpus variation snapshot (MO2 subset, 35 saves with `SAVEGAME.SAV`):
- Distinct observed gold values (`SVIT.current_count[2]`, payload `0x036`): `0, 19, 53, 85, 95, 110, 181, 202, 244`.
- Distinct inferred location codes from `SVMD` heuristic: `?ask, ?bas, ?isz, ?ith, ?leg, ?who` (plus some unmatched/empty cases).

Thumbnail field pattern (current 37-save corpus):
- All scanned saves still contain `THMB` at canonical offset `0x118`.
- `THMB` field bytes immediately after tag are uniformly `00 01 80 00` (`be u32 = 0x00018000 = 98,304`).
- In-game and inspector behavior matches this: thumbnail appears corrupted, while save loads otherwise normally.
- Treat this as a **non-fatal thumbnail-data anomaly** in current saves, not a general save-container failure.

Per-sample snapshot (parser-heuristic extraction):

| Save | Size (bytes) | Gold (`SVIT.current_count[2]`) | Health Potions (`SVIT.current_count[4]`) | Active Loc Id (`SVMD`) | Inferred Loc Code (`?xxx`) |
|---|---:|---:|---:|---:|---|
| `SAVEGAME.001/SAVEGAME.SAV` | 1,678,028 | 53 | 6 | 1 | `?smk` |
| `SAVEGAME.002/SAVEGAME.SAV` | 1,678,268 | 53 | 6 | 1 | `?smk` |
| `SAVEGAME/SAVEGAME.003/SAVEGAME.SAV` | 776,416 | 53 | 6 | 1 | `?smk` |
| `SAVEGAME/SAVEGAME.004/SAVEGAME.SAV` | 773,458 | 53 | 6 | 1 | `?smk` |
| `SAVEGAME/SAVEGAME.005/SAVEGAME.SAV` | 1,678,292 | 51 | 6 | 1 | `?leg` |

Table notes:
- `Gold` and `Health Potions` are item-table `current_count[n]` values inside `SVIT`:
  - Gold = item `2` at payload `0x036`
  - Health Potions = item `4` at payload `0x062`
- `Inferred Loc Code` uses current `SVMD` heuristic mapping (`u32 id at token-4`, token pattern `\?[A-Za-z]{3}`), then matches active id from final `u16` of payload.

Known-tag occurrence validation (`SVGM`, `THMB`, `SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVIT`, `SVCB`, `SVMV`, `SVMD`):
- Every scanned save contains **exactly one** occurrence of each known tag listed above.
- Full tag ordering is identical across all `37` scanned saves.
- For every first-seen tag, `offset + 8 + be_length` stayed within file bounds (bounded length sanity passed).
- No unexpected chunk tags were observed in the current corpus.

Notable variability observed across saves (first-seen BE length field):
- Stable lengths in current corpus:
  - `SVGM=272`, `THMB=98,304`, `SCR3=614,400`, `SVGF=1,849`, `SVCM=1,016`, `SMFM=7,168`, `SVG2=1,666`, `SVIT=1,896`, `SVCB=32`, `SVMV=15`.
- Variable lengths in current corpus:
  - `SMGR`: `32,812..36,164`
  - `SMOB`: `19,980..859,464`
  - `SMRP`: `0..1,988`
  - `SMCO`: `624..50,544`
  - `SMSH`: `198..564`
  - `SVRG`: `1,692..15,956`
  - `SVGV`: `1,542` (stable)
  - `SVMD`: `9..519`

Practical implication:
- The current “known tags but not decoded” blocks are strongly confirmed as real/active save structures, not theoretical inventory entries.
- A next decode pass should prioritize variable-length blocks (`SMGR`, `SMOB`, `SMRP`, `SVRG`) because they likely carry gameplay-state deltas.
- Save-slot discovery logic should keep distinguishing:
  - SAV-backed slots (`SAVEGAME.SAV` present), and
  - TSG-only directories (present in corpus, but currently treated as non-canonical/incomplete unless runtime evidence proves standalone loadability).

## Cross-Block Relationships
- `SVMD` is the primary structural location-identity signal.
- Exact `SAVEGAME.SAV` to `.TSG` `SVMD` equality is treated as an internal identity match, not a
  user-facing name by itself.
- When multiple `.TSG` files share the same `SVMD`, current code narrows them by:
  1. exact `SVCB`
  2. unique `SVCB word5`
  3. unique `SVCB word3`
  4. exact `SVRG`
- In the current local corpus, that stack resolves every observed structurally ambiguous slot to
  one `.TSG` basename.
- `SVIT` must not be interpreted the same way across `SAVEGAME.SAV` and `.TSG`; only the
  `SAVEGAME.SAV` interpretation is currently confirmed.
- User-facing location is **not** currently treated as solved:
  - subtitle-derived evidence paths (`SVMD` labels, parsed map labels, transition-derived
    comments) are useful for RE
  - but they can still surface non-location strings such as `BYE`, `HELP`, person names, and
    other topic/dialogue text
  - therefore the normal visible `Location` field should stay blank unless a future derivation
    path is proven structurally
- The parser scans all matching `*.TSG` sidecars in the save folder and keeps a best-effort `Area`
  token for developer details only.
- Current evidence suggests the common `Area` token family (for example `HBDOCK06`) corresponds to
  real map object/reference identifiers in `ISLAND` assets, but not yet a proven user-facing
  location by itself.
- Chunk extraction helper (`findLastChunkPayload`) applies “last bounded-valid occurrence wins,”
  affecting all parsed blocks (`THMB`, `SVIT`, `SVMD`).

## Parser Entry/Construction Flow (Implementation Detail)
`RedguardsSaveGame` constructor sequence:
1. `resolveSavePath()`
2. `scanAuxiliaryFiles()`
3. `detectSlotFromFolder()`
4. `parseSaveHeader()`

Notable behavior:
- `m_PCName` is forced to `Cyrus`.
- Save display name prefers:
  - parsed save title
  - folder-derived display name
  - fallback `Redguard Save`

Endianness rule (current parser path):
- chunk length is interpreted as BE `u32` during chunk scan; field endianness inside payloads is block-specific.

Chunk selection rule used by parser helpers (`findLastChunkPayload`):
- scan begins at configured minimum offset (current calls use `0`),
- every tag occurrence is tested with bounds check (`dataPos + len <= fileSize`),
- truncated/out-of-bounds occurrences are skipped,
- extraction uses the **last bounded-valid** occurrence in file order.

Practical implication:
- late duplicate chunks override earlier valid chunks for metadata extraction,
- malformed trailing chunk headers do not invalidate earlier valid payloads.

Save-path resolution rule (`resolveSavePath`):
- if input is a slot directory, parser prefers `SAVEGAME.SAV`,
- if missing, falls back to first `*.SAV` file in name order,
- auxiliary file scan excludes `SAVEGAME.SAV` and records all other files in slot folder.

## Blocks

### SVGM
- Status: implemented (signature-level)
- Structure:
  - File offset `0x00..0x03`: expected literal `SVGM`.
- Known Fields:
  - Signature validity flag set from `bytes.mid(0, 4) == "SVGM"`.
- Notes:
  - Parser continues extracting some metadata even when not fully validated as canonical format.

### Header Region (pre-chunk metadata used by parser)
- Status: implemented
- Structure:
  - Offset `0x08`, max len `8`, C-string: format/version string.
  - Offset `0x14`, max len `128`, C-string: save title.
- Known Fields:
  - `FormatVersion`: fixed C-string at `0x08`.
  - `SaveTitle`: fixed C-string at `0x14`.
- Notes:
  - C-string extraction truncates at first `\0`, then trims whitespace.

### THMB
- Status: implemented (high confidence for current corpus)
- On-disk Structure (working):
  - `Tag[4]`: literal `THMB`.
  - `Field0[4]`: payload length (`BE u32`) in all currently scanned saves.
  - `Payload[...]`: image bytes.
- Current corpus model:
  - `Field0 = 0x00018000` in every scanned save.
  - Payload decodes as fixed `256x192` RGB565 (LE 16-bit pixels), matching `98,304` bytes.
- Known Technical Details:
  - Known valid screenshot decode in corpus: `256x192`, RGB565, payload size `0x18000`.
  - Dimension sanity constraints used by current tooling:
    - width/height in `1..2048`.
- Required Validation Rules (format-level):
  - `THMB` header and payload must remain within file bounds.
  - `payloadOffset + payloadLength <= fileSize`.
  - Reject truncated/overflowing payloads.
- Implementation Notes (separate from format spec):
  - Current parser probes canonical `THMB` location near `0x118` first, then falls back to full scan.
  - Current parser uses RGB565 fast path when `LenBE == 0x18000`, then retains a defensive fallback decode path.
- Open Questions:
  - Are non-`0x18000` THMB payloads used in broader/late-game save corpora, or is current fallback path purely defensive?

#### Unknowns
- No unresolved THMB structural fields in currently scanned corpus.
- Broader corpus may still reveal variants not present in current local set.

### THMB Corruption Tolerance (Observed Runtime Behavior)
- Reported manual spot checks:
  - Multiple saves with visibly corrupted THMB previews still load in-game.
  - No immediate non-thumbnail save issues were observed for those samples.
- Working implication:
  - `THMB` should be treated as non-critical preview metadata for validity decisions.
  - Parser/UI should warn on thumbnail decode failure, but must not classify slot as broken on THMB failure alone.

Additional implementation details:
- Signature finder first checks canonical offset `0x118`; if absent, it scans whole file for a bounded valid `THMB` chunk.
- Screenshot decode routine (`fetchDataFields`) chooses decode mode by payload header sanity:
  - Mode A: `rawLenBE == 0x18000` => decode `256x192` RGB565.
  - Mode B: treat header bytes as `width/height` (LE `u16/u16`) and decode planar RGB if bounds-valid.

### SVIT
- Status: structurally mapped for `SAVEGAME.SAV`; runtime parser still consumes only selected fields
- Structure:
  - Parser finds last valid `SVIT` chunk in file.
  - Requires payload length `>= 0x64` for current field extraction.
  - Confirmed `SAVEGAME.SAV` payload length is `0x768` (`1896`) bytes.
  - Item records cover IDs `0..85` inclusive.
  - Record size is `0x16` (`22`) bytes.
  - Record base formula:
    - `record_base(n) = n * 0x16`
  - Payload ends with a trailing `LE u32 = 1` at payload offset `0x764`.
- Confirmed per-record layout (`SAVEGAME.SAV` only):
  - `base + 0x00`: `u16`, unknown / reserved
  - `base + 0x02`: `LE u32` -> `total[n]`
  - `base + 0x06`: `LE u32` -> `player_max[n]`
  - `base + 0x0A`: `LE u32` -> `current_count[n]`
  - `base + 0x0E`: `byte[8]`, unknown / reserved
- Confirmed `ITEM.INI` relationship:
  - `SVIT.total[n]` <-> `ITEM.INI total[n]`
  - `SVIT.player_max[n]` <-> `ITEM.INI player_max[n]`
  - `SVIT.current_count[n]` = serialized player-held count at save time
- Known user-facing item mappings:
  - Gold = item `2` current count = payload `0x036`
  - Potion of Ironskin = item `3` current count = payload `0x04C`
  - Health potion = item `4` current count = payload `0x062`
  - Aloe = item `44` current count = payload `0x3D2`
  - Torch = item `45` current count = payload `0x3E8`
  - Canah Feather = item `54` current count = payload `0x4AE`
  - Strength potion = item `67` current count = payload `0x5CC`
- Notes:
  - Controlled probe-save validation confirmed the record stride and field identities by forcing
    `ITEM.INI total[n]`, `player_max[n]`, and `player_total[n]` to `n + 0x71`, then creating a new
    save and observing that `SVIT.total[n]`, `SVIT.player_max[n]`, and `SVIT.current_count[n]`
    matched that pattern exactly for item IDs `0..85`.
  - Item ID `86` (`LAST` in retail `ITEM.INI`) does not appear as a normal `0x16`-byte record in
    the confirmed `SAVEGAME.SAV` layout.
  - `SVIT` is not globally uniform across all save-side files. The chunk also appears in many `TSG`
    files, where these same offsets can hold context-specific values rather than the player’s global
    inventory counters.
  - Full ID/name/offset mapping table is maintained in `blocks/SVIT.md`.

#### Unknowns
- Actual player health/state values remain unresolved; current `SVIT` coverage only exposes potion counts.
- Meaning of the per-record `u16` at `base + 0x00`.
- Meaning of the per-record trailing `byte[8]` at `base + 0x0E`.
- Meaning of the final trailer `LE u32 = 1` at payload offset `0x764`.
- Whether `TSG` `SVIT` chunks reuse this exact layout.

#### REDEDIT editor corroboration (new evidence)

- `REDEDIT` is an external item editor whose binary contains the literal string `SVIT` and multiple editor error messages:
  - `Unable to find Item Data in File`
  - `Unable to find any Items`
  - `You Must Open a File First`

Implications:
- This ties the editor directly to the `SVIT` chunk, corroborating that `SVIT` is the editor's target for item/inventory edits.
- The editor exposes exactly four editable item values (captions observed in the UI):
  - `Gold Ammounts` (Gold)
  - `Health Potions`
  - `IronSkin Potions`
  - `Strength Potions`
- From import-table analysis and VB6 runtime usage, the editor performs positional file I/O using 4-byte reads/writes (`__vbaGet4`, `__vbaPut4`, `__vbaFileSeek`), indicating the editor targets 4-byte integer fields (u32) rather than single-byte counters.
- The editor-visible values now map cleanly to confirmed `current_count[n]` fields:
  - Gold -> item `2` -> `SVIT+0x036`
  - IronSkin Potions -> item `3` -> `SVIT+0x04C`
  - Health Potions -> item `4` -> `SVIT+0x062`
  - Strength Potions -> item `67` -> `SVIT+0x5CC`

REDEDIT-derived candidate constants (observed in the editor write path):
- `0x003C`
- `0x0052`
- `0x0068`
- `0x05D2`

Notes on candidates:
- These constants appear in the editor's write logic and are plausibly related to the four editable values, but their exact meaning is unresolved: they might be transformed offsets, absolute offsets in the full save after locating item data, item identifiers/indices, or positions inside an intermediate hex/text buffer used by the editor.
- They do **not** match the canonical payload offsets directly (`0x036`, `0x04C`, `0x062`, `0x5CC`).
- Do NOT treat these as confirmed `SVIT+offset` mappings yet. Document them as REDEDIT-derived candidates and prioritize empirical testing.

Spec updates applied here:
- Mark `SVIT` as a `SAVEGAME.SAV` item-table chunk with explicit listing of the four editor-supported editable values:
  - Gold
  - Health potion count
  - Potion of Ironskin count
  - Strength potion count
- Note that the editor's failure messages imply a minimal item-recognition phase (the editor searches/recognizes item data regions rather than blindly reading a couple of fixed offsets).
- Note the editor's hex-display workflow ("Convert to Intel Hex", "RichTextBox"), which suggests it may operate on a transformed hex/text view when locating and writing back item data rather than strictly using fixed absolute offsets.

Recommended next steps (for researchers):
- Add a small instrumented save fixture and use the editor to perform a controlled write while monitoring the modified bytes to resolve the candidate constants.
- Validate whether `TSG` `SVIT` chunks reuse the `SAVEGAME.SAV` per-item record layout or only share the tag name.

Extraction path details:
- Parser uses **last valid** `SVIT` chunk in file (`findLastChunkPayload`).
- Chunk framing interpreted as `Tag + be u32 length + payload`.

### SVMD
- Status: implemented (heuristic)
- Structure:
  - Parser finds last valid `SVMD` chunk in file.
  - Active location id is read from final 2 bytes of payload (`u16`, LE).
  - Parser regex-scans payload text view for tokens matching `\?[A-Za-z]{3}`.
- Known Fields / Heuristics:
  - For each token match at position `pos`:
    - location id read from 4 bytes at `pos - 4` (`u32`, LE).
    - token normalized to lowercase 3-letter code (leading `?` removed).
  - Active location id is mapped to its token code when available.
  - Code is resolved to human-readable subtitle via `ENGLISH.RTX` lookup key `?xxx`.
- Notes:
  - This is a practical heuristic, not fully proven canonical field layout.

#### Unknowns
- Exact canonical `SVMD` internal layout (token table, id map semantics) remains unresolved.

Extraction path details:
- Parser uses **last valid** `SVMD` chunk in file.
- Active location id is read from `payload[payloadSize-2..payloadSize-1]` as LE `u16`.
- Candidate code table is built from regex token matches in payload text view.

### Current Parser Coverage Boundary (Source-backed)
- In `RedguardsSaveGame` implementation, the only hardcoded legacy save chunk tags currently referenced are:
  - `THMB` (thumbnail detection/extraction)
  - `SVIT` (gold/potion fields)
  - `SVMD` (location-code heuristic)
- No direct tag handlers currently exist in this parser for:
  - `SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVCB`, `SVMV`
- Practical meaning:
  - these tags are present and length-bounded in corpus validation,
  - but currently treated as opaque payloads by runtime save-metadata parsing.

### Chunk Table Dumper (Implemented Utility)
- `RedguardsSaveGame::chunkTableDebugText()` returns a tab-delimited chunk table from current `SAVEGAME.SAV`.
- Output columns:
  - `tag`
  - `tag_offset`
  - `payload_offset`
  - `length_be`
  - `bounds` (`ok` or `out-of-bounds`)
  - `decode_status` (`parsed`, `known-opaque`, `unknown`)
- Current `decode_status=parsed` set is intentionally limited to active handlers:
  - `THMB`, `SVIT`, `SVMD`
- This utility is read-only and does not modify save files.

### SCR3
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory; no current parser extraction.

### SVGF
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SMGR
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: present in corpus scans; currently no direct parser handler in `RedguardsSaveGame`.

### SMOB
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: present in corpus scans; currently no direct parser handler in `RedguardsSaveGame`.

### SMRP
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: present in corpus scans; currently no direct parser handler in `RedguardsSaveGame`.

### SMCO
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SMSH
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SVCM
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SMFM
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SVRG
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: present in corpus scans; currently no direct parser handler in `RedguardsSaveGame`.

### SVGV
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SVG2
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SVCB
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

### SVMV
- Status: known tag, not decoded
- Structure: unknown.
- Known Fields: none.
- Notes: listed in external docs/tag inventory.

## Auxiliary Files (TSG and related)

### TSG Token Heuristic (current parser)
- Status: implemented (heuristic)
- Structure:
  - parser scans first matching `*.TSG` file in save folder.
  - regex token extraction: `\b([A-Z]{2}[A-Z]{4}\d{2})\b`.
- Known Behavior:
  - excludes `NCPLAK01` token.
  - scores candidates and keeps best token as area indicator.
- Notes:
  - This is a helper heuristic for display, not a full TSG decoder.

Scoring specifics used by current parser:
- Base score starts at `0` per token.
- `+10` if token starts with `HB`.
- `+5` if token occurrence count `<= 3`, otherwise subtract occurrence count.
- Highest-scoring token selected.

## Location Label Resolution (RTX-backed)
- Parser resolves location code to subtitle via `ENGLISH.RTX`:
  - loads RTX entries through `RedguardsRtxDatabase`
  - maps `?code` key to subtitle text
- Candidate codes are prioritized by:
  - explicit active `m_LocationCode`
  - additional deduplicated `m_LocationCodes`
- Weak-code filtering exists to avoid low-quality matches.

Weak token sets currently hardcoded:
- weak codes: `bye`, `mus`, `leg`, `isz`
- weak subtitles: `BYE`, `MUSIC`, `LEAGUE`, `ISZARA`

Fallback order:
1. first non-weak RTX subtitle
2. first RTX subtitle
3. uppercase raw code

## File Enumeration Behavior
- `allFiles()` returns `SAVEGAME.SAV` and ensures the original filepath is included.
- Auxiliary files are tracked separately (`m_AuxiliaryFiles`) by slot-folder scan.

## What Is NOT Yet Fully Parsed
- No complete decode of all known chunk families (`SCR3`, `SVGF`, `SMGR`, etc.)
- No formal decoded model for:
  - full player stats/inventory/equipment sets
  - script/quest progression structures
  - full world/object simulation state

This means the current parser is best described as a **high-value metadata parser**, not a full state deserializer.

## Discrepancies / Competing Theories
- Theory A: `THMB` post-tag field is payload length (`BE u32`) with fixed RGB565 payload (`256x192`).
- Theory B: `THMB` post-tag field is dimensions (`LE u16/u16`) with planar RGB payload.
- Current evidence: all currently scanned local saves support Theory A (`THMB` at `0x118`, field=`0x00018000`, bounded payload).
- Parser behavior recommendation (current): keep Theory B as defensive fallback only until a real counterexample is observed.

## Implementation Status (Plugin)
- Implemented:
  - signature/version/title extraction (`SVGM` + header-region fields),
  - thumbnail extraction (`THMB`) with guarded dual decode path,
  - selected `current_count[n]` extraction from `SVIT` in `SAVEGAME.SAV`
    (`Gold`, `Potion of Ironskin`, `Health Potion`, `Strength Potion`, `Aloe`, `Torch`, `Canah Feather`),
  - location-code heuristic + RTX-backed label resolution via `SVMD`,
  - slot + auxiliary file enumeration and chunk-table dump utility.
- Partial:
  - `SVMD` structural semantics,
  - `TSG` area-token heuristics for display.
- Not implemented:
  - full decode for known opaque tags (`SCR3`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SVCM`, `SMFM`, `SVRG`, `SVGV`, `SVG2`, `SVCB`, `SVMV`),
  - full gameplay-state reconstruction (inventory/quest/script VM/object state).

## Confidence Register
- High confidence:
  - signature/header extraction offsets (`SVGM`, version/title)
  - full current-corpus chunk order and bounded framing (`37` files)
  - THMB field/length model in current corpus (`0x00018000`, RGB565 `256x192`)
  - `SAVEGAME.SAV` `SVIT` item-table layout:
    - payload length `0x768`
    - item IDs `0..85`
    - record stride `0x16`
    - `total[n]`, `player_max[n]`, `current_count[n]` field offsets
- Medium confidence:
  - meaning of the `SVIT` per-record unknown prefix/tail bytes
  - meaning of the final `SVIT` trailer `u32`
- Low confidence:
  - whether `TSG` `SVIT` chunks share the same semantics
  - SVMD token/id mapping heuristics
  - TSG area token scoring heuristics

## Open Questions / RE Backlog
1. Capture sample corpus with path-level expected fields to lock offsets.
2. Add dedicated decoders for non-implemented tags in priority order after corpus profiling.
3. Validate whether any non-`0x18000` THMB variants exist in broader save corpora.

## References
- `src/games/redguard/redguardsavegame.h`
- `src/games/redguard/redguardsavegame.cpp`
- `src/games/redguard/gameredguard.cpp`
- `src/games/redguard/redguardsrtxdatabase.h`
- `src/games/redguard/redguardsrtxdatabase.cpp`
