# XnGine archive integration and format contracts

This document describes the current C++ implementation in `src/xngine/`, not
a guarantee that every game installation or archive variant has been validated.
It concerns the DOS-era XnGine container supported by `XngineBSAFormat`;
the shared container reader does not interpret the contents of game-specific
record payloads.

## Responsibilities

- `XngineBSAFormat` reads and writes the common count/type/data/footer layout,
  presents named or numeric entries, and exports/imports files and a manifest.
- Each derived `GameXngine` plugin supplies `bsaTraits()` and
  `bsaFileSpecs()`. Traits govern header leniency, names, compressed records,
  and variant hints. A file spec identifies an expected archive name and, only
  when known, its index type.
- `XngineArchiveExtractorFeature` adapts those operations to MO2. Its
  filename filters are derived from extensions in the game's declared specs.
  They are UI discovery filters, **not a complete format allowlist**.

`supportsArchive` attempts to parse an existing archive, including an
unrecognized basename, using the game traits. For known basenames the adapter
also applies the declared variant hint. Conversely, `canCreateArchive`
currently returns true for a *known basename*, whereas the underlying
`packKnownXngineBsaArchive` separately rejects a spec with
`indexTypeKnown = false`. **The creation eligibility check does not itself
guarantee the pack operation will succeed.** For example, Arena declares
`GLOBAL.BSA` without a known index type.

## Container layout

The reader expects a little-endian record count and normally a type header:
`NameRecord` (`0x0100`) or `NumberRecord` (`0x0200`). If the game's
traits permit a missing type header and the interpreted type is unknown,
the reader falls back to a two-byte count-only header and `NameRecord`.
This is a bounded compatibility fallback, not a complete automatic
identification of all headerless formats.

Record payload bytes occupy the area before a fixed-size descriptor footer.
The selected descriptor layout controls how the same descriptor widths are
interpreted:

- The Daggerfall candidate layout uses a 14-byte name plus signed payload size
  for named records, or a 32-bit record ID plus size for numbered records. It
  has no compression field.
- The Battlespire candidate layout uses a 12-byte name, 16-bit compression
  flag, and size for named records, or a 16-bit record ID, compression flag,
  and size for numbered records.

`DescriptorLayout::Auto` currently chooses the Battlespire layout when the
Battlespire LZSS mode is selected and the Daggerfall layout otherwise. That is
a compatibility rule, not format autodetection. Callers can select the layout
explicitly. The parser rejects negative lengths, footer overlaps, record data
past the footer, and data/footer length mismatches. The `Snd` and `Sav`
variants describe shared container/index conventions; the game-specific audio
or save payload meanings belong to their consumers.

Daggerfall's current traits require DOS 8.3-compatible, uppercase names
and do not permit compressed entries. Battlespire's traits permit its
LZSS decoder, compressed pass-through *writes*, and the missing-type-header
fallback. The rules are per game, not global assumptions about every BSA.

## Compression and the manifest

When a Battlespire-layout entry marked as compressed is read, its bytes are
either:

1. Preserved as-is when `allowCompressedPassthroughRead` is enabled, or
2. Decompressed using the game's configured decompression mode.

For pass-through reads, the entry retains its nonzero `compressed` flag and the
payload remains encoded. For decoded reads, the reader clears the flag because
the in-memory bytes are no longer compressed. This prevents a later write from
labeling decoded bytes as encoded data.

The writer **does not compress input**. A nonzero compression flag is accepted
only for the Battlespire descriptor layout when compressed pass-through writes
are explicitly enabled; the supplied bytes must already contain a valid encoded
payload. Daggerfall descriptors reject nonzero compression flags because that
layout has no field in which to serialize one. Never interpret the existence of
a manifest as proof of a byte-identical compressed round trip.

Extraction writes files into the requested directory and creates
`xngine_bsa_manifest.tsv` with the columns:

```text
ordinal	type	name	record_id	compressed	source_file
```

Named records use sanitized filenames and acquire numeric suffixes when
there are case-insensitive collisions; numbered records use generated
`<id>_<ordinal>.bin` names. The manifest preserves the record name/ID,
compression marker, source filename, and original row order. Extraction
overwrites the manifest and may overwrite record files. It is not
transactional and does not roll back partially written output on failure.

Packing from a plain directory reads the nonrecursive files in filename
order. For `NumberRecord` input, each input file's complete basename must
parse as a numeric ID. The numbered names generated by extraction therefore
normally require **manifest-based** packing, not directory-only packing.
When the manifest exists, `GameXngine::packKnownXngineBsaArchive` uses it
instead of the plain directory path. The manifest reader consumes rows in
order and uses their name/ID, compression marker, and source filename.
The ordinal and type columns are not independently validated against the
requested output index type.

Only use trusted, inspected input directories and manifests. The current
writer opens the output in truncate mode before completing all validation
and writes entries incrementally; failed operations may leave an incomplete
output archive rather than preserving a previously existing file.

## Scope of verification

The observations above are derived from the current `GameXngine`,
`XngineBSAFormat`, and `XngineArchiveExtractorFeature` code. This
documentation does not change parsing or packing behavior, establish a
new licensing policy, or assert a comprehensive format round-trip test suite.
