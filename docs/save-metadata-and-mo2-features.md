# Save metadata and MO2 feature boundaries

This guide documents the shared XnGine adapter. Game-specific save
parsers interpret individual formats; the shared file reader does not
establish that every game uses all of its generic compression or
plugin-list features.

## Slots and optional metadata

`XngineSaveGame` represents a directory slot or a single-file save.
For directory slots it lists immediate files. For single-file saves it
can add a script-extender companion when the MO2 feature is present and
the corresponding file exists. Game parsers supply player and location
details. Screenshot and legacy plugin fields are fetched lazily through
`fetchDataFields()`; the base implementation returns empty data.

The generic `FileWrapper` contains raw, zlib-chunk, and LZ4 readers.
This does not mean that every XnGine title uses those compression
layouts. The LZ4 path currently does not check the decompressor return
value, so successful return from the wrapper alone does not establish
a validated decoded byte count. For zlib, chunk continuation uses the
compressed byte count consumed by inflate, aligned to a 16-byte
boundary in the file.

## Save tooltip and missing assets

`XngineSaveGameInfoWidget` clears metadata, screenshots, and dynamic
details when selecting another save. Classic `SAVE<n>` and Redguard
`SAVEGAME.<n>` names are recognized for display without changing the
underlying paths.

`XngineSaveGameInfo::getMissingAssets()` returns an empty result
unconditionally. This is an unimplemented dependency check, not evidence
that all referenced files are present.

## MO2 plugin and unmanaged-mod adapters

`XngineGamePlugins` implements MO2's plugin-list interface as no-ops,
returning empty plugin/load-order lists. This differs from MO2 mod
priority and the game-specific file override and patch systems.

`XngineUnmanagedMods` likewise does not enumerate unmanaged files.
An empty result is not an audit of the game directory.

No executable behavior was changed in this documentation pass.
