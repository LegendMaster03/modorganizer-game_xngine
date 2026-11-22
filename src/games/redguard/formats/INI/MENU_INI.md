# MENU.INI

Last updated: 2026-03-11

## Role
Menu layout/configuration INI.

Retail path:
- `Redguard/MENU.INI` (`42023` bytes in current scan)

## Implementation Coverage
- Included in plugin `iniFiles()` set.
- Unity/reference parser has typed handling for `[general]` + `[page0]..[page7]` with menu item geometry/action fields.
- Workspace C++ does not currently deep-parse menu fields.

## Typed Key Coverage (Unity reference)

### Sections
- `[general]`
- `[page0]..[page7]`

### General keys
- `preload_sprites`, `hi_res_menu`, `ignore_autosave`, `force_movies`

### Per-page indexed keys
- texture refs: `texture_set[index]`, `texture_index[index]`
- menu item fields: `text_x`, `text_y`, `text`, `justify`, `selectable`, `default_selected`, `texture`, `action`, `grayed`, `output_x`, `output_y`, `slider_min`, `slider_max`

## Parser Behavior Notes
- `io_tst` route parses `[general]` through page handler for page `0` while still setting global values.
- Key parsing is split by first `=` pair in reference parser path; additional unescaped `=` are not retained there.
