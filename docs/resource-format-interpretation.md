# Resource format parsing: interpretation and known limits

The XnGine resource readers separate structural parsing from claims about
content compatibility or lossless binary round trips. Game-specific wrappers
may select variants explicitly through traits rather than rely on filename
extensions or automatic detection.

## WLD world data

`XngineWldFormat` handles both Daggerfall woods and Redguard world-map
payloads. Automatic variant detection uses file sizes and plausible header
values. Redguard world data contains four sections, each with four 128x128
byte planes behind a big-endian section header. Unexpected surplus bytes
may produce a warning rather than a hard failure. Do not interpret detection
success as proof that every original metadata field is understood.

## TEXT.RSC resources

`XngineRscFormat` recognizes a table of six-byte text record headers with a
sentinel. It bounds records using sorted offsets, handles text control tokens,
and warns about invalid headers or skipped records. Its writer rebuilds
record offsets in order from supplied raw record bytes; that does not
establish byte-for-byte equivalence to every original database.

## Palettes

`XnginePaletteFormat` distinguishes 768-byte raw RGB palettes and 776-byte
headered palettes, with a caller-controlled trailing-data fallback. RGB
values are interpreted as six-bit DAC intensities only when **every**
channel is at most 63; otherwise the entire palette is treated as eight-bit.
This is a content-based heuristic, not per-color format detection.

## Redguard GXA images and ROB meshes

`RedguardsGxaFormat` expects BMHD metadata, BPAL palette, BBMP pixel data
and an END marker. Type 0 BBMP images contain raw indexed pixels; type 1
uses a scan for plausible embedded headers; type 2 currently accepts only
a single image. Some parsed image data remains encoded: successful
container parsing is not equivalent to successful pixel decoding.

`RedguardsRobFormat` parses a ROB segment table and then attempts to
interpret embedded mesh payloads or external 3DC references relative to a
supplied directory. Unresolved external files, unsupported segment modes,
and unparsed meshes are retained as warnings. A parsed ROB container does
not prove that all referenced geometry is present.

## Redguard FNT fonts and RTX subtitle/audio databases

`RedguardsFntFormat` reads FNHD metadata, a palette (BPAL/FPAL), FBMP
glyph data, and END. Glyph widths/heights bound the indexed pixel
slices. RDAT is optional unless the header declares it; when its
structure is not recognized, the raw block is retained without
inventing field meanings.

`RedguardsRtxDatabase` retains a separate entry-order list and a
label-keyed map. Writing rebuilds the reverse lookup index after
the END marker. Reading can stop on a truncated entry while returning
the entries decoded so far, so a successful return is not an
end-to-end integrity check. Sidecar changes use labels for existing
entry updates; the numeric index chooses the new-entry branch and
does not serve as a general reordering command.

## Fixed lookup tables

`XngineTblFormat` supports an exact 256-byte lookup table. A TBL
file is not automatically a palette: the parser returns uninterpreted
byte values, and automatic recognition is size-based.

## Daggerfall IMG and related image formats

`Daggerfall::Image` recognizes several legacy raster/container families.
IMG RLE decoding is separate from identifying a plausible header.
Two specifically named IMG files (`FRAM00I0.IMG` and `TALK00I0.IMG`) are
treated as uncompressed for the observed `0x0800` header value. Other
unsupported compression values retain undecoded bytes with a warning;
their pixel payloads should not be presented as fully decoded.

These notes document current implementation heuristics. Comprehensive
format support requires representative original assets, malformed-file
tests, and save/extract/repack comparisons, not only source inspection.
## Game-specific water-table wrappers

`DaggerfallWaterTbl` selects the shared `ByteLut256` format explicitly.
`BattlespireWaterTbl` currently exposes the same operation only as a
Daggerfall-derived research adapter: no original Battlespire `WATER.TBL` file
or production caller has been established. Candidate water values are also
projected from BS6 `WATR` chunks, but that separate interpretation still needs
original-asset validation. The 256-byte adapter preserves uninterpreted values;
it does not establish what an entry means for either game. The TBL writer
validates the size before opening the target file, but it does not roll back a
subsequent write failure.
