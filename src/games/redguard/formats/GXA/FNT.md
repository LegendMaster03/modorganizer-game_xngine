# FNT (Redguard Fonts)

Last updated: 2026-03-11

## Scope
Fonts are stored in `FONTS/*.FNT`.
Not all fonts are used; usage varies by Glide vs Software mode.
Some fonts do not include lowercase characters.

## Byte Order
- Section length fields are big-endian (`be u32`).
- Per-field numeric encoding inside blocks follows documented per-field types.
- Java parser implementation reads `RDAT` size via `DataInputStream.readInt()` (big-endian) and glyph width/height via little-endian helpers.

## Container Layout
Repeated section structure until terminator:
- `SectionName` (`char[4]`)
- `DataLength` (`be u32`)
- `SectionData` (`DataLength` bytes)

Terminator:
- `END ` (4-byte marker only; no length/data)

## Section Order
1. `FNHD`
2. `BPAL` or `FPAL`
3. `FBMP`
4. `RDAT` (optional, only if `FNHD.HasRDAT == 1`)
5. `END `

## Blocks

### FNHD
- Status: documented
- Size: 56 bytes
- Fields:
  - `Description` (`char[32]`)
  - `Unknown_24` (`u16`) values observed `0,1,3`
    - reported corpus pattern:
      - `0`: `REDLOW` and ARIAL-family fonts
      - `1`: most other fonts (and reported to correlate with no `RDAT` in that set)
      - `3`: `REDGUARD`
  - `HasRDAT` (`u16`) `1` if `RDAT` exists
  - `Unknown_28` (`u16`) always `0`
  - `Unknown_2A` (`u16`) always `0`
  - `Unknown_2C` (`u16`) always `0`
  - `MaxWidth` (`u16`) (possibly max glyph width; usage unclear)
  - `LineHeight` (`u16`)
  - `CharacterStart` (`u16`) observed `20` (space)
  - `CharacterCount` (`u16`)
  - `Unknown_36` (`u16`) always `0`
  - `Unknown_38` (`u16`) always `0`
  - `Unknown_3A` (`u16`) always `1`

### BPAL / FPAL
- Status: documented
- Size: 768 bytes
- Fields:
  - 256 RGB triplets (`u8,u8,u8`)
  - Components are 6-bit range (`0..63`) and scale by `*4` for 8-bit display.
- Notes:
  - Color index `0` used as transparency channel key.
  - `FPAL` appears to be an alias tag; structure matches `BPAL`.

### FBMP
- Status: documented
- Size: variable
- Model:
  - Character count comes from `FNHD.CharacterCount`.
  - Characters are in sequential ASCII order starting at `FNHD.CharacterStart`.
  - Each character:
    - 10-byte char header
    - indexed pixel payload `Width * Height`
- Per-character fields:
  - `IsEnabled` (`u16`): `0` disables glyph, `>0` enables
  - `OffsetLeft` (`s16`)
  - `OffsetTop` (`s16`)
  - `Width` (`u16`)
  - `Height` (`u16`)
  - `CharacterData` (`u8[Width*Height]`), top-left origin

Reported corpus behavior for `IsEnabled`:
- ARIAL-family + `REDGUARD` + `REDLOW`: sequential non-zero IDs starting at `1`.
- many other fonts: boolean-like `0/1` usage.
- runtime-relevant rule is consistent: `0` hidden, `>0` visible.

Parser-derived loading detail (Java reference tool):
- after `FBMP` section header, records are scanned by first 4 bytes:
  - starts with `'E'` => end.
  - starts with `'R'` => `RDAT` follows.
  - otherwise treated as glyph record.
- glyph parse path then skips 2 bytes and reads `Width/Height` (`u16 le`) before reading indexed pixels.

### RDAT
- Status: documented optional block
- Size: 173 bytes
- Notes:
  - Reported export metadata block, not used by runtime.
  - Exists only when `FNHD.HasRDAT == 1`.
  - Java parser treats `RDAT` as opaque payload: reads section size (`be u32`) then consumes bytes without decoding fields.

External field map (reported/inferred):
- `0x00` `char[136]` `SourceFile`
- `0x88` `u32` `Unknown_90`
- `0x8C` `u32` `Unknown_94`
- `0x90` `u32` `Unknown_98` (reported `0`)
- `0x94` `u32` `Unknown_9C` (reported `2` on `ARIALBG`, `0` otherwise)
- `0x98` `u32` `Unknown_A0` (reported near `MaxWidth`)
- `0x9C` `u32` `Unknown_A4` (reported near char height/line metrics)
- `0xA0` `u32` `Unknown_A8` (reported range `1..3`)
- `0xA4` `u32` `Unknown_AC` (reported range `1..2`, possibly style-related)
- `0xA8` `u32` `Unknown_B0` (reported `0`)
- `0xAC` `u8`  `Unknown_B4` (reported `0`)

Validation status:
- runtime-unused behavior is tested/observed.
- individual RDAT field semantics remain inferred.

### END
- Status: documented
- Size: 0 bytes
- Marker: `END `

## Rendering Notes (Behavioral)
- Character allotment: `(Width + 1) x LineHeight`.
- Space character receives extra width buffer (reported):
  - small (`LineHeight < 16`): `+3`
  - medium (`LineHeight == 16`): `+4`
  - large (`LineHeight > 16`): `+8`
- Drop shadows are runtime-generated, not stored in glyph bitmap.
  - reported behavior: render glyph at `(+1,+1)` with near-black translucent color beneath main glyph.
  - reported note: console text path may omit the extra space-buffer behavior.

## Similarities to Shared XnGine Patterns
- 4CC + big-endian chunk-length framing.
- 768-byte RGB palette payload compatible with shared palette conversion logic.

## Discrepancies
- Tag alias discrepancy: `BPAL` vs `FPAL`.

## Unity Parser Cross-Check
- Unity importer uses strict chunk iteration: `FNHD`, `BPAL|FPAL`, `FBMP`, optional `RDAT`, then `END `.
- `FNHD` fields map directly to a 56-byte payload and include:
  - `firstCharacter` (`s16`)
  - `numCharacters` (`s16`)
- `FBMP` records are parsed by iterating exactly `FNHD.numCharacters` character entries.
- Each character record is read as fixed 10-byte metadata + `width*height` indexed bytes.
- `RDAT` is handled as opaque bytes (size-driven), matching current documentation.

## Unity `io_tst` Validation Corpus Notes
- `io_tst/textst/fnttst.cs` iterates a broad retail font corpus (`FONTS/*.FNT`) and logs `FNHD.description` for each file.
- Corpus includes menu/UI variants and color/style families such as:
  - `ARIAL*`, `FONT*`, `HI*`, `LO*`, `RED*`, `SRED*`.
- Practical implication: parser assumptions (`FNHD` decode + `FNHD.numCharacters`-driven FBMP iteration) were exercised against many real Redguard fonts.

## Reverse-Engineering Priorities (from io_tst)
1. **`FNHD.unknown1..unknown7` semantic labeling** (Highest value)
  - These are consistently parsed in Unity and likely contain style/layout flags.
  - Candidate mapping path: correlate values across known UI font families (`HI*`, `LO*`, `RED*`, `SRED*`).
2. **`RDAT` payload structure decode**
  - Unity/parser currently treats RDAT as opaque bytes.
  - Comment hints (`char[136]` description + trailing integers) need confirmation against corpus.
3. **Glyph metric behavior validation**
  - Quantify how `offsetLeft/offsetTop/maxWidth/lineHeight` interact in runtime placement for menu text.

## Current Gap Summary
- Known-good: chunk framing, glyph record decoding, palette path, character iteration.
- Unknown: most FNHD unknown fields + RDAT internals.

## External Documentation Merge Notes
The following items were merged from external reference documentation:
- explicit `Unknown_24` value-family mapping,
- `IsEnabled` pattern differences across font families,
- full RDAT offset table hypothesis,
- additional notes for space padding and shadow compositing behavior.

## Confidence + Outstanding Verification
- High confidence: container framing, glyph record layout, palette interpretation path, optional `RDAT` handling.
- Medium confidence: merged behavioral notes (`Unknown_24` family patterns, `IsEnabled` usage families).
- Needs confirmation: fine-grained `RDAT` field semantics and several `FNHD` unknown-field labels.

## Related Format Docs
- `TEXBSI/TEXBSI.md` (model texture container)
- `COL.md` (external palette fallback used by TEXBSI/textures)
- `RGM/RGM.md` (map/object/script container consuming model/render assets)

## Additional Implementation Notes
- Java texture loader reads first 72 bytes before palette decode (covers `FNHD` + `FPAL` framing in that tool path).
- Palette decode in Java loader multiplies source components by `4`.
- Java writer path for textures (shared with TEXBSI workflow) uses exact palette index lookup and falls back to nearest RGB in-map when color is missing.

## Sources
- `redguard-mod-manager-main/src/redguard/TextureFile.java`
- `redguard-mod-manager-main/src/redguard/Texture.java`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGFNTFile.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/textst/fnttst.cs`
