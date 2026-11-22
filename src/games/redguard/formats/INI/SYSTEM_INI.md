# SYSTEM.INI

Last updated: 2026-03-11

## Role
Primary runtime configuration INI in the game root.

Retail path:
- `Redguard/SYSTEM.INI` (`5231` bytes in current scan)

## Observed/Reported Sections
External notes list:
- `[screen]`, `[system]`, `[debug]`, `[game]`, `[cyrus]`, `[3dmanager]`, `[xngine]`, `[camera]`, `[dialog]`, `[3dfx]`

## High-Value Key Groups (reported)
- File indirection: `rtx_filename`, `world_ini`, `item_ini`, font/bitmap paths
- Frame/audio controls: `normal_frame_rate`, `min_frame_rate`, `max_frame_rate`, `volume`, `redbook`, `redbook_volume`
- UI placement: `compass_*`, `candle_*`, `logbook_*`, `pickup_*`
- Startup/runtime checks: `disable_drive_check`, `disable_cpu_check`, `disable_svga_check`, `report_machine`
- Object/effect budgets: `max_active_objects`, `max_effects`, `max_particles`, `max_remap_objects`

## Section Theme Notes (external, preliminary)
- `[screen]`: display/movie toggles and palette adjustment values
- `[system]`: core content pointers, frame bounds, feature toggles, screen-anchor coordinates
- `[debug]`: debug mode/logging and error-recovery toggles
- `[game]`: gameplay thresholds (fall damage, dialog/combat radii)
- `[cyrus]`: player-control tuning (turn, walk/auto-defend, camera distance)
- `[3dmanager]`: memory/cache configuration
- `[xngine]`: software-render path parameters (planes, detail, sky/haze)
- `[camera]`: camera offsets/limits/smoothing/combat/rope profiles
- `[dialog]`: dialog UI positioning and interaction limits
- `[3dfx]`: hardware-render variant menu/text settings

## Unit Conventions (reported)
- time often described as game ticks (possibly around `~18.2 Hz`),
- screen coordinates in pixels for `640x480` baseline,
- distance/angle fields use engine-specific units (not plain SI/degrees).

## Implementation Coverage
- Included in plugin `iniFiles()` list for profile/mod management.
- No deep typed parser currently implemented in workspace C++ for full `SYSTEM.INI` semantics.

## Caveat
Most key semantics come from behavior notes and remain partially speculative.
