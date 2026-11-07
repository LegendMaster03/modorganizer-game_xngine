# Binary format interpretation and compatibility fallbacks

The format modules beneath `src/xngine` and `src/games` serve two layers:
`XngineBSAFormat` reads the outer record container, while game-specific
parsers interpret a record's internal binary layout. A container that reads
successfully does not imply its payload has a recognized schema.

## Daggerfall BLOCKS.BSA

`DaggerfallBlocksBsa` dispatches named records by extension:

- `RMB` has a fixed 6,776-byte header, up to 32 block pairs, model and
  flat lists, and optional trailing bytes. Each block is bounded using its
  declared size, while its exterior/interior objects have counted entries.
  Truncated sections cause errors; nonzero pointers, empty blocks, or
  trailing content can instead produce diagnostic warnings.
- `RDB` stores model references and a root grid of linked object lists.
  The parser tracks visited offsets to stop cycles or duplicate traversal.
  It builds a second index and action-link graph only after walking the
  objects. Missing targets and undecodable action references are recorded as
  warnings, not invented as resolved relationships.
- `RDI` retains a 512-byte payload and derives summary statistics.

The parser retains raw bytes where the internal semantics are incomplete.

## Daggerfall MAPS.BSA

`DaggerfallMapsBsa` joins `MAPNAMES`, `MAPTABLE`, and `MAPPITEM`
entries by their three-digit region suffix and corresponding record order.
Location-name resolution uses decoded 16-bit location numbers. A secondary
lookup uses the low 16 bits of a map ID for save-header compatibility, with
the first mapping retained on collisions. That secondary association is
inferred, not a guaranteed unique map identifier.

Nearest-location resolution caches coordinate entries by the absolute
MAPS.BSA path and compares both coordinate orientations using Manhattan
distance. It rejects matches past a broad fixed distance threshold.
This is a heuristic and the path-keyed cache is not automatically invalidated
when the archive changes on disk.

## Shared 3D mesh records

`Xngine3dFormat` interprets multiple 3D version tags (v2.5, v2.6, v2.7,
v4.0, and v5.0). The v2.x parser interprets vertex references as offsets
with version-specific scaling and reconstructs UV coordinates from the first
four stored anchors when possible. It tries a four-anchor affine solution
and then a three-anchor planar fallback for additional vertices; an
unresolved UV does not imply the entire mesh is invalid.

The v4/v5 parser uses 32-bit texture references and different face headers
and vertex index semantics. Its auxiliary UV blocks are parsed separately.
Do not assume a v2.x offset convention applies to v4/v5.

`Battlespire3dBsa` first attempts the shared parser. For v2.7 records whose
10-byte face-header layout aligns with the recorded normal offset, it tries
two **heuristic** 8-byte normalization strategies and then parses the
normalized copy. The chosen compatibility path is surfaced as a warning.
It should not be described as a proven repair for every Battlespire model.

## Redguard RGM map rewriting

`RedguardsMapFile` reads four-character section tags followed by
big-endian signed lengths, stopping at `END `. It stores decoded records,
then initializes scripts/headers from RAHD, RAST, RASB, RAVA, RASC and RAAT.
A short or malformed section ends scanning; the current reader can still
return success with the records found so far. This is not strict end-to-end
integrity validation.

Writing rebuilds several scripted sections from parsed text and header data,
reuses a subset of original sections in specified order, and retains the
presence/absence of optional RAVC. Rewriting is **not** proven byte-for-byte
lossless, especially for unrecognized sections. Existing section/header
alignment assumptions should be tested before treating rewritten maps as
round-trip-equivalent to their sources.

These caveats describe existing behavior and do not modify binary readers
or writers. Testing against representative original and modified archives
remains necessary before claiming comprehensive format support.
