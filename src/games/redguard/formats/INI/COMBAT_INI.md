# COMBAT.INI

Last updated: 2026-03-11

## Role
Combat behavior/lookup configuration.

Retail path:
- `Redguard/COMBAT.INI` (`54069` bytes in current scan)

## Implementation Coverage
- Included in plugin `iniFiles()` set.
- Not deeply parsed in current workspace C++ map database path.
- Referenced semantically by RGM attributes (attack/defend IDs in `RAAT` behavior notes).

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `54069` bytes
- section count: `474`

Observed section families:
- `misc` (`1` section)
- `attackNN` (`89` sections: `attack00..attack88`)
- `defendNN` (`5` sections: `defend00..defend04`)
- `tauntNN` (`379` sections across sparse IDs in `0..599`)

## Section Key Taxonomy

### `misc` key set
- `sword_clank_1`, `sword_clank_2`
- `non_engaged_dist`, `in_face_dist`, `non_engaged_spacing`, `in_combat_threshold`
- `player_can_die`, `cyrus_defend_interval`, `node_timer`, `max_taunts`

### `attackNN` key union
- core shape/timing: `type`, `arc`, `arc_center`, `min_range`, `max_range`, `first_collide_frame`, `collide_duration`
- effects/results: `damage`, `force`, `defense`, `first_defend_frame`, `defend_duration`, `animation`
- extra metadata seen in some sections: `elevation`, `col_vertex`, `col_sphere_size`, `num_attacks`, `attack00`, `attack01`, `attack02`

### `defendNN` key union
- `arc`, `min_range`, `max_range`, `first_collide_frame`, `collide_duration`, `animation`

### `tauntNN` key union
- `type`, `animation`, `rtx_label`

## Indexing Patterns
- `attackNN` and `defendNN` appear contiguous and dense in observed ranges.
- `tauntNN` is sparse: many IDs in `0..599` are intentionally absent.
- sample gap ranges include:
	- `12..49`, `134..149`, `231..234`, `255..259`, `297..299`, `399..408` (not exhaustive)

## Embedded Comment Signals
- Header comments map animation names to numeric IDs (for example defend/attack animation aliases).
- These comments are useful RE hints but are not currently consumed by workspace parser code.

## Usage Link to RGM
- RGM behavior notes reference attack/defend IDs in `COMBAT.INI` for NPC fight attributes.
- See `../RGM/blocks/RAAT.md` for attribute-side mapping context.

## Related Docs
- `../RGM/blocks/RAAT.md`
