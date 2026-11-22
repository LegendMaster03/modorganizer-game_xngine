# surface.ini

Last updated: 2026-03-11

## Role
Surface/material mapping style INI.

Retail path:
- `Redguard/surface.ini` (`4255` bytes in current scan)

## Implementation Coverage
- Included in plugin `iniFiles()` set.
- No dedicated deep parser currently documented in workspace C++.

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `4255` bytes
- section count: `13`

Sections observed:
- `[surfaces]`, `[unknown]`, `[water]`, `[deepwater]`, `[scapewater]`, `[scapedeepwater]`, `[lava]`, `[sand]`, `[wood]`, `[tile]`, `[scape]`, `[rock]`, `[gloop]`

Section key-shape summary:
- no assignments observed in: `[surfaces]`, `[unknown]`, `[lava]`, `[scape]`, `[gloop]`
- numeric-key mappings observed in material sections (`4`, `5`, `6`, `7`, `8`, `9`, `10`, `11`, and optionally `14`)

Examples from scan:
- `[water]` / `[scapewater]`: `4..11 => 69`, `14 => 70`
- `[deepwater]` / `[scapedeepwater]`: `14 => 70`
- `[wood]`: `4..11 => 89..96`, `14 => 97`
- `[tile]`: `4..11 => 110..115`, `14 => 116`
- `[sand]`: `4..11 => 106..109` (alternating)
- `[rock]`: `4..11 => 102/103` pattern

## Cross-Doc Correlation
- mapped values align with SFX effect index ranges documented for material footsteps/landings.
- see `../SFX.md` and `../SFX_EFFECTS_FULL.md`.

## Caveat
Current page documents observed mapping structure and value patterns from retail file scan; exact runtime selection logic for each numeric key is not yet traced in workspace code.
