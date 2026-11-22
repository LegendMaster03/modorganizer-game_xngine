# ITEM.INI

Last updated: 2026-03-11

## Role
Item registry, labels/descriptions, and item metadata.

Retail path:
- `Redguard/ITEM.INI` (`30512` bytes in current scan)

## Implementation Coverage (workspace C++)
- `RedguardsMapDatabase::readItemsFile(...)` currently consumes line pairs:
  - `name=...`
  - following `description=...`
- Used to construct item text IDs for map/script content workflows.

## Additional Typed Coverage (Unity reference)
- typed item key family includes global and per-item indexed keys.

### Global keys
- `bitmap_file`, `bitmap_selected_file`
- `start_item_list`, `start_item_select`
- `additional_length`, `weapon_sphere_size`, `default_weapon_item`, `torch_time`

### Per-item indexed keys
- core fields: `type`, `flags`, `hide`, `total`, `player_max`, `player_total`
- text fields: `name`, `description`
- script fields: `use_script`, `script_instances`
- art/model fields: `bitmap`, `inventory_object_file`, `game_object_file`, `hand_object_file`, `hilt_object_file`
