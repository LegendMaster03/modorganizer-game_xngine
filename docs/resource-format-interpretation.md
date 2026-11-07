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
