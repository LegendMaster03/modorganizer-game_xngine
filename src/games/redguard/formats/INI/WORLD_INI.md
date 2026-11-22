# WORLD.INI

Last updated: 2026-03-11

## Role
World/map registry and environment parameters.

Retail path:
- `Redguard/WORLD.INI` (`18913` bytes in current scan)

## Implementation Coverage (workspace C++)
- `RedguardsMapDatabase::readWorldFile(...)` currently parses:
  - `world_map[index]=<path/name>`
- Used to build map ID/name mapping for RGM processing pipeline.

## Additional Typed Coverage (Unity reference)
- parser includes many typed `world_*` keys and vectors.

### Global keys
- `start_world`, `start_marker`, `test_map_delay`, `test_map_order`

### Per-world indexed keys
- map/asset links: `world_map`, `world_world`, `world_palette`, `world_sky`, `world_skyfx`, `world_flash_filename`, `world_sunimg`
- scalar environment values: `world_redbook`, `world_shade`, `world_haze`, `world_ambient`, `world_background`, `world_compass`, `world_ambientfx`, `world_sunangle`, `world_sunskew`, `world_sunscale`, `world_sky_xrotate`, `world_sky_yrotate`, `world_skyscale`, `world_skylevel`, `world_skyspeed`, `world_rain_delay`, `world_rain_drops`, `world_rain_start`, `world_rain_end`
- vector keys parsed from comma lists:
  - `world_sun` (`vec4`)
  - `world_rain_sphere1..4` (`vec4`)
  - `world_ambientrgb` (`vec3`)
  - `world_sunimgrgb` (`vec3`)
  - `world_sunrgb` (`vec4`)
  - `world_fogrgb` (`vec3`)
  - `world_wave` (`vec3`)

## Related Docs
- `../WLD.md`
- `../RGM/RGM.md`
