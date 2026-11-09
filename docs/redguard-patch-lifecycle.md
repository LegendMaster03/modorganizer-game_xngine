# Redguard patch staging lifecycle

This document describes the current Redguard plugin implementation in
`src/games/redguard/redguardspatchruntime.cpp` and its supporting utilities.
It documents existing behavior; it is not a proposed replacement patch pipeline.

## Selection and precedence

`applyRedguardPatchMods` asks MO2 for
`allModsByProfilePriority()`. `scanRedguardPatchMods` keeps only active mods
that contain an enabled patch type:

- `INI Changes.txt`, `Map Changes.txt`, and `RTX Changes.txt` use the
  `allowDillon241` flag.
- Recursive `*.xdelta` files use `allowExeMods`.

A mod with both kinds is included once. If no eligible patch mod is found,
the function returns without preparing a temporary output mod. The list
and last selected priority are retained for the later staging pass.

## Generated output and lifetime

The output is named `__redguard_patch_output_<profile name>`, using
`Default` when no profile name is available. The plugin attempts to remove
the prior output directory, then creates or reuses the MO2 mod entry,
activates it, and places it immediately after the last selected patch mod
by priority. Its files are **generated output**, not the source mods.

The existing directory-removal failure is logged but does not abort setup.
If MO2 fails to create the mod entry, the code can still create an output
directory, but warns that the files may not be activated in MO2. Generation
is not transactional, and a failed operation may leave partial output.

A `onFinishedRun` callback deactivates the output mod and removes its
directory. **Current limitation:** a process-wide `static bool` registers
that callback only once; its captured path and mod name come from the first
registration. This should not be read as a guarantee of correct cleanup
across multiple profiles within one plugin process. This document does not
change that behavior.

## Application order

`applyRedguardPatchModsInOrder` visits the selected mods in MO2 priority
order. For each mod, it processes enabled INI/RTX changes and collects
map-change records. It also stages audio and supported art/texture folders.
Where a game installation has multiple matching renderer art roots,
`stageDirectoryVariants` fans out the applicable source directories to
those roots within the generated output.

If executable patches are enabled, matching `*.xdelta` files are applied
using the resolved xdelta tool. The plugin first uses a configured
nonempty `xdelta_exe_path` and stores that last configured value through
`QSettings`; if the current plugin setting is blank, it reads the stored
global fallback. Tool discovery and patch application then take place
per selected mod.

Map-change records from selected mods are accumulated in order and applied
once after the per-mod pass. This is different from applying each mod's
map changes independently.

A failed file copy, format edit, or xdelta operation generally marks the
result unsuccessful without undoing earlier staged files. In particular,
`copyDirectoryContents` removes a destination file before replacing it;
a failed replacement is not rolled back. Investigate the generated output
and `[GameRedguard]` log messages after an unsuccessful run instead of
assuming the directory represents a complete result.

## Implementation boundaries

- `redguardspatchruntime.cpp` selects mods, schedules output cleanup, and
  orders the per-mod operations.
- `redguardspatchutils.cpp` resolves base files and performs file-based
  staging and format edits.
- `redguardsmapchanges.cpp` accumulates map edits for the final apply step.
- `xngineexepatch.cpp` discovers and invokes the external xdelta tool.

These are observable implementation contracts and caveats, not proof that
every possible Redguard installation or mod combination has been tested.
