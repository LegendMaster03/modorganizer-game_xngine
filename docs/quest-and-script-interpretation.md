# Quest and script format interpretation

The Daggerfall QBN and Redguard RGM script modules contain partial reverse-
engineered descriptions of game data. This document distinguishes readable
binary structure from verified runtime semantics.

## Daggerfall QBN

`DaggerfallQbn` reads a 60-byte header containing section record counts
and offsets. It sorts valid offsets to determine section boundaries and
uses the available byte count, rather than trusting declared record counts,
to size fixed-record sections. Mismatches and trailing bytes can result in
warnings. Some section types are preserved raw because their fields have
not been fully identified. Section 10 contains optional debug text-variable
names; its absence does not necessarily invalidate the rest of the quest.

`DaggerfallQbnPseudo` recognizes fixed-size pseudo-code instruction
records with up to a bounded number of operand subrecords. Known opcode
names and argument ranges are documentation for decoding; an unknown opcode
or unusual argument count is reported without implying complete knowledge
of the in-game operation.

## Redguard RGM scripts

`RedguardsScriptParser` depends on map, item, function, and flag catalogs
loaded by `RedguardsMapDatabase`. It strips trailing source comments and
substitutes recognized symbolic identifiers before generating binary
instructions. Label references can be written as placeholders and resolved
after the containing script block has been compiled. Conditional blocks
similarly backpatch their end offset after the nested block is emitted.

`RedguardsScriptReader` performs the reverse interpretation to readable
text. Unknown function IDs can appear as placeholder names; this is not
proof the original program had that spelling. The result is a decoded
representation whose fidelity depends on the correctness of the catalogs
and the subset of instructions implemented.

## Map changes

`RedguardsMapChanges` represents edits by map name and source-line
position. An initial literal `null` entry means delete the original
line, followed by any added replacement lines in their stored order.
This is distinct from an empty string, which is a retained insertion.
The RGM writer applies the collected changes to a regenerated script,
not by patching unchanged binary byte offsets. Source-line edits therefore
depend on the script reader producing compatible line positions.

None of these facilities is a sandbox for executing untrusted quest
scripts or a complete verified compiler for every game format. No
executable behavior has been modified in this documentation pass.
