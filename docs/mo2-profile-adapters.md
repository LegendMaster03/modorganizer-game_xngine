# MO2 profile integration and archive-list helpers

XnGine exposes several MO2 feature adapters. Their presence in the source
tree does not mean every game plugin registers each feature. The game
plugins and `GameXngine` determine registration.

## Profile-local saves

`XngineLocalSavegames` maps an MO2 profile save directory to the game save
directory returned by `GameXngine`, provided the target directory exists.
Unlike newer Bethesda game integrations, `prepareProfile` performs no
INI-based local-save switch. The game-specific slot layout, physical
directory creation, and legacy save discovery are handled by the shared
game adapter; see its comments and declared `SaveLayout` contract.

## Archive-list storage

`XngineDataArchives` offers a common wrapper around Windows INI archive
list settings. It parses comma-separated archive names and delegates
persistence to a derived `writeArchiveList` implementation. A zero-length
read does not distinguish an absent key from every possible API failure.

Archive additions check for existing names case-insensitively. **Current
limitation:** removal first checks case-insensitively but uses
`QStringList::removeAll`, which compares exact case, so a different-case
entry can remain in the list. This documentation does not change that
behavior.

## Classic BSA invalidation helper

`XngineBSAInvalidation` is a classic INI/dummy-BSA adapter. Both
`activate` and `deactivate` call `prepareProfile`; the current
`profile->invalidationActive()` value determines which archive/INI
operations are performed. The INI target is profile-local when local
settings are enabled, and the game's documents directory otherwise.

The helper also has a process-static cached invalidation archive name
initialized from the first instance's virtual name. This is not scoped
independently to each derived game class. Its declaration and implementation
alone do **not** establish that a given XnGine title currently registers
or uses this feature.

These notes describe existing behavior, limitations, and adapter
boundaries. No executable changes have been introduced.
## Auxiliary feature adapters and archive tools

`XngineScriptExtender` and `XngineUnmanagedMods` implement MO2 feature
interfaces but provide intentionally inactive behavior in the shared
implementation. They do not report an installed extender or enumerate
unmanaged mods. An empty unmanaged-mod result therefore **does not**
prove that an installation has no files outside MO2.

`DummyBSA` is legacy Mod Organizer code for a one-folder, one-file
Bethesda-style invalidation archive. Its synthetic `dummy.dds` has no
payload, and the caller supplies the BSA version and output path. It
is **not** a packer for the DOS-era XnGine BSA record formats described
in the archive guide. The current writer does not propagate file-open
or write errors to its caller.

The standalone `tools/bsa_extract_cli` chooses Battlespire BSA traits
and applies an explicit sound-archive hint for `SPIRE.SND`. It is an
extraction utility, not an automatic multi-game variant detector or an
archive-creation test.
