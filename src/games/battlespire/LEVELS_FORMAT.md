# Battlespire `LEVELS.TXT` Field Notes

This documents the current understanding of each column in `LEVELS.TXT`.

## Row Layout

Each row has 12 tab-separated columns:

1. `MapFile`  
Example: `l1.bs6`, `dmnn.bs6`.  
Confidence: high.

2. `MapCode`  
Example: `Lv1..Lv7`, `Dm1..Dm7`, `Cf1..Cf2`.  
Likely the internal short code shown/used by game systems.  
Confidence: high.

3. `ValueA`  
Example: `1024`, `1336`, `1500`, `2000`, `3500`.  
Unknown numeric gameplay/render parameter.  
Confidence: low.

4. `ModeFlags`  
Example: `NNF`, `NYF`, `NYC`, `NNC`, `DNF`, `DNC`, `CNC`, `CNF`.  
Likely mode/environment flags (campaign/deathmatch/capture + visual mode).  
Confidence: medium for "flags", low for exact meaning.

5. `SkyOrBackdrop`  
Example: `sky1.bsi`, `l2sky.bsi`, or `xxxx` (none).  
Confidence: high.

6. `ValueB`  
Usually `555` or `444`.  
Unknown numeric parameter, probably visual/environment related.  
Confidence: low.

7. `ValueC`  
Example: `1024`, `2048`, `3072`, `4000`, `31000`.  
Unknown numeric parameter, likely draw/fog/range related.  
Confidence: low.

8. `NodesFile`  
Example: `nodes.l1`, `nodes.m3`, or `xxxx` (none).  
Likely AI/path node data file.  
Confidence: high.

9. `ValueD`  
Example: `1000`, `1500`, `2000`, `2500`.  
Unknown numeric parameter.  
Confidence: low.

10. `ValueE`  
Example: `2048`, `3072`, `4096`, `6000`.  
Unknown numeric parameter.  
Confidence: low.

11. `ValueF`  
Example: `55`, `60`, `80`, `100`.  
Unknown numeric parameter (possibly difficulty/recommendation/scoring).  
Confidence: low.

12. `DisplayName`  
Example: `The Weir Gate`, `Deathmatch level 1`.  
Human-readable level name.  
Confidence: high.

## What This Means For Save Parsing

- `SAVEVARS.DAT` currently provides a numeric `CurrentLevel`.
- `LEVELS.TXT` has no explicit numeric-ID column.
- Current mapping assumes `CurrentLevel` uses 1-based row order.
- Campaign entries (`Lv1..Lv7`) are verified enough for display.
- Multiplayer entries (`Dm*`, `Cf*`) are still inferred from row order and should be validated with real saves.

## Validation Plan

1. Collect one save per known map/mode where possible.
2. Read `CurrentLevel` from `SAVEVARS.DAT`.
3. Verify against the expected `LEVELS.TXT` row.
4. Promote inferred mappings to confirmed mappings.
