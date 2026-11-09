# Executable patching: contracts and failure boundaries

This describes the **current** XnGine plugin implementation. It does not
approve binary patches, guarantee compatibility with a specific game version,
or change the patching pipeline.

## Responsibility and selection

`src/xngine/xngineexepatch.cpp` implements shared JSON byte edits and
xdelta, IPS, BPS, UPS, and PPF file patching. The game-specific plugin owns
which formats are enabled, where patch files are discovered, and when
output is staged. In Daggerfall, `GameDaggerfall::applyExePatchMods()` is
compiled only when `XNGINE_DAGGERFALL_EXE_PATCHING` is defined; it visits
active mods in MO2 profile-priority order and creates a profile-named
temporary output mod placed after the last contributing mod.

Daggerfall collects per-mod inputs before applying them. Its per-mod
execution order is replacement executable, JSON catalog, then xdelta, IPS,
BPS, UPS, and PPF, with each patch-file group sorted case-insensitively.
The temp mod supplies the current working executable when available; otherwise
the installed game supplies the source. The finished-run hook deactivates
and removes the generated output. It is **not** an all-or-nothing transaction:
failures are counted and logged while some other stages can proceed, and
previously staged files are not rolled back.

`DaggerfallFallExeHacks` is a separate Daggerfall-specific JSON/catalog
interface; its built-in hardcoded patch sets were intentionally retired.
Shared `XngineExePatch` handles the execution path used by the staging
pipeline. Do not equate the existence of metadata such as `targetVersion`
with runtime verification that the executable actually has that version.

## JSON byte patches

Patch catalogs contain named sets of offsets, replacement bytes, and
optional expected bytes. Offsets may be nonnegative numeric values or
strings parsed as integers. The loader rejects empty identifiers, missing
patch data, negative offsets, and expected/replacement length mismatches.

`applyPatchSets` checks all patches in one set against the **current**
working byte array, then writes that set's changes before processing the
next set. A mismatch in a later set can leave earlier sets applied to the
in-memory buffer. When `expected` is omitted, only bounds and basic
format checks protect the byte replacement. Callers must supply a
version-compatible source and meaningful expected bytes where possible.

## Binary format matching

`apply*PatchToAnyFileInTree` looks for candidate files under the staged
output and original game roots. For a matching relative path, it prefers the
already staged file, allowing successive patches to build on earlier output.
The patched result is written to a temporary file and then renamed into
the generated mod; it does not intentionally edit the installed base file.

- **xdelta** invokes an external executable with `-d -s`. The helper
  tries available candidates; its candidate set is not explicitly ordered
  by filename preference. A successful decoder run is the main indicator
  that a source file matched. Tool lookup checks configured, mod, game,
  MO2, environment, and PATH locations; process execution has a 120-second
  wait limit.
- **IPS** reads literal and run-length encoded offset records. It has
  no mandatory source-file checksum, so a syntactically applicable patch
  is not proof that the chosen input was correct.
- **BPS** and **UPS** validate CRC32 checksums for the source, patch, and
  generated output. Their target-tree helpers score likely candidate
  filenames before trying them; checksum validation is the actual
  compatibility test. BPS target-copy operations are bytewise to preserve
  overlapping-copy semantics.
- **PPF** handles versions 1, 2, and 3 and the optional description footer;
  its code recognizes PPF3 undo data and advances past it when applying.
  Although it checks for the presence of a PPF2 binblock or PPF3 blockcheck
  section, this implementation does **not** compare their bytes to the
  source file. Candidate matching is heuristic rather than authenticated
  source verification.

All helpers may leave partial or replaced generated output on a failed
write/rename, and the wider multi-mod sequence has no rollback.

## Maintenance checklist

If a new patch format or matching strategy is introduced, verify these
independent concerns rather than assuming one proves the others:

1. Gate the patch format through its game's explicit setting and staging path.
2. Preserve profile-priority order, correct relative output paths, and
   earlier staged results for subsequent patches.
3. Check source compatibility independently of patch-file syntax, with
   checksums or expected bytes when the format permits.
4. Keep the game's original installation untouched by using temporary
   generated output, and report cleanup failures accurately.
5. Test successful application, wrong-source rejection, malformed inputs,
   and failure cleanup on the relevant Windows/MO2 installation.

The repository currently has no automated workflow covering these cases.
