# RTX (Dialogue + Optional Audio)

Last updated: 2026-03-08

## Scope
RTX files (example: `ENGLISH.RTX`) store dialogue label entries with subtitle text and optional embedded audio payload.

## Container Layout
Primary entry stream:
- Repeated records until label `END `.
- Record framing:
  1. `Label` (`char[4]`)
  2. `TotalLength` (`be u32`) for this entry payload
  3. `HasAudio` (`u16`): observed values `0` or `1`
  4. `SubtitleLength` (`u32 le`)
  5. `Subtitle` (`char[SubtitleLength]`, IBM437 in tooling)
  6. optional audio metadata + audio bytes (when `HasAudio == 1`)

## Audio Metadata Block (when HasAudio == 1)
In order:
1. `DoubleSizeA` (`u32 le`)
2. `DoubleSizeB` (`u32 le`) (same value as `DoubleSizeA` in writer)
3. `SampleRate` (`u32 le`) (observed 11025 or 22050)
4. `Unknown_0C` (`u32 le`) (writer constant `100`)
5. `Unknown_10` (`u16 le`) (writer constant `0`)
6. `Unknown_12` (`s32 le`) (writer constant `-1`)
7. `AudioLength` (`u32 le`)
8. `Unknown_1A` (`u8`) (writer constant `0`)
9. `AudioBytes` (`u8[AudioLength]`)

Notes:
- `DoubleSize* == 1` is used by the reference tool to indicate 22050Hz/16-bit-expanded path.
- Non-double 22050 and 11025 entries are also supported by the reference tool.

Tool interpretation notes:
- `sampleRate=22050` + `doubleSize=1` => 16-bit-like expanded path in tool playback conversion.
- `sampleRate=22050` + `doubleSize=0` => alternate 8-bit path converted to external signed output format.
- default/no-audio constructor path assumes 11025 semantics.

## Trailer Index (RNAV)
After `END `, a reverse-ordered index is written by the reference tool:
- For each entry (reverse order):
  - `Label` (`char[4]`)
  - `EntryPosition` (`u32 le`)
  - `EntryLength` (`u32 le`)
- Footer:
  - `RNAV` (`char[4]`)
  - `TotalBytes` (`u32 le`) across primary stream including `END `
  - `EntryCount` (`u32 le`)

## Byte Order
- Mixed-endian:
  - entry `TotalLength` is read/written as big-endian in reference implementation.
  - most scalar metadata inside records is little-endian.

## Unity Parser Cross-Check
- Unity importer reads `Label` as raw `u32` and compares against `0x20444E45` (`END ` in little-endian memory order) to terminate entry parsing.
- Unity importer checks `HasAudio == 256` for audio-bearing records, which is equivalent to a big-endian `0x0001` flag interpreted via little-endian `ReadUInt16()`.
- Subtitle bytes are decoded as ASCII in Unity test/import code paths.
- Unity importer computes audio payload length from `TotalLength - SubtitleLength - 31` before reading audio bytes, matching a fixed metadata overhead model.
- Unity importer stores entries in dictionary keyed by integer label value (packed 4-char code), not by string label text.
- Unity parser reads per-entry `TotalLength` field but item decode proceeds by explicit field reads rather than direct seek by this length.

## Implementation-Derived Constraints
- Labels are treated as 4-character keys.
- Subtitle bytes are stored raw without null termination.
- Audio metadata block contributes fixed `27` bytes plus raw audio payload in writer length calculations.
- Strings are read/written using IBM437 in the Java toolchain (`Utils.readString`/`writeString`).
- Writer emits both forward entry stream and RNAV trailer index; reader in this tool primarily consumes forward stream and stops at `END `.

## `RTX Changes.txt` Sidecar (Tooling)
Java mod-manager tooling uses a tab-delimited sidecar file named `RTX Changes.txt`:
- Per-line format: `index<TAB>label<TAB>subtitle`
- `subtitle` may be omitted; parser then uses empty string.
- Merge application is load-order sequential across enabled mods.

Observed parser behaviors:
- `index` is parsed as decimal integer.
- In `RtxDatabase.applyChanges`, when `index >= currentEntryCount`, a new entry is inserted using `label` + `subtitle`.
- Otherwise, existing subtitle is updated by `label` lookup (`rtxEntryMap.get(label)`), not by positional index.

Editor parity notes:
- `DialogueEditor` writes only modified rows in the same tab-delimited shape.
- Empty sidecar files are deleted when saving to a mod folder.

Implementation caveat (current Java mod-manager apply flow):
- During global mod apply, `RTX_AUDIO_FOLDER` overlays are loaded into base `rtxDatabase`, while written output is either backup copy (no text changes) or `modifiedDatabase` clone (text changes).
- As implemented, audio-folder-only changes and audio overlays paired with subtitle changes may not propagate to final `ENGLISH.RTX` output in that path.

## Java Audio Conversion Notes (`RtxEntry`)
`RtxEntry` distinguishes three practical audio payload modes:
- `11025` base path
- `22050` non-double path
- `22050 + doubleSize==1` path (`16`-bit-like expanded storage)

Tool constants used in Java playback/export path:
- `AUDIO_FORMAT_11025`: `11025`, sample-size `4`
- `AUDIO_FORMAT_22050`: `22050`, sample-size `8`
- `AUDIO_FORMAT_22050_DOUBLE`: `22050`, sample-size `16`

Behavioral implications:
- external playback/export stream is converted to signed little-endian mono (`externalAudioFormat()`).
- when input WAV is imported, `22050` is preferred only if source sample-rate is already `22050`; otherwise tool favors `11025` conversion paths.
- entry length math remains container-driven (`6 + subtitleLen + optional(27 + audioLen)`) regardless of audio mode.

## Sources
- `redguard-mod-manager-main/src/redguard/RtxDatabase.java`
- `redguard-mod-manager-main/src/redguard/RtxEntry.java`
- `redguard-mod-manager-main/src/redguard/Utils.java`
- `redguard-mod-manager-main/src/modManager/RedguardModManager.java`
- `redguard-mod-manager-main/src/modManager/DialogueEditor.java`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGRTXFile.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/sndtst/rtx2int.cs`
- `redguard-unity-master/redguard-unity-master/io_tst/sndtst/rtxtst.cs`
