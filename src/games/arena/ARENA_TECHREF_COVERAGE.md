# Arena Tech Reference Coverage

This file tracks what from the current Arena technical reference is implemented in the MO2 Arena plugin today.

## Implemented

- `GLOBAL.BSA` archive support wiring for Arena (`GameArena::bsaFileSpecs`).
- Arena install layout detection for Steam/GOG/native DOS layouts.
- Arena executable discovery (`ARENA.EXE` / `A.EXE`) and DOSBox launch entries.
- Arena save slot mapping and virtualization for:
  - `SAVEENGN.0x`
  - `LOG.0x`
  - `SPELLS.0x`
  - `NAMES.DAT`
- `SAVEENGN.0x` parsing (documented encrypted fields):
  - Level (`0x06`)
  - Experience (`0x0409`)
  - Gold (`0x041E`)
  - Blessing (`0x0422`)
  - Detail raw value (`0x0B90`)
- Main quest flag parsing:
  - Semantics for `0x0DD2`, `0x0DD3`, `0x0DD4`, `0x0DDB`
  - Raw flag block exposure for `0x0DD2..0x0DDB` in save details.
- `LOG.0x` parsing for entry count and latest title.
- `SPELLS.0x` parsing for spell record counts and preview.
- Save naming via `NAMES.DAT`.

## 1.0 MVP Status

The remaining practical Arena work is narrower than the earlier gap list:
- The mod checker recognizes the known Arena save bundle stems (`AUTOMAP`, `IN`, `LOG`, `SAVEENGN`, `SAVEGAME`, `SPELLS`, `STATES`, `WILDPAL`) and `NAMES.DAT`.
- Arena loose-file acceptance includes `ADV`, `ASI`, and `AVI` where those archives are encountered.
- Stability testing remains the main operational gap for repeated install, uninstall, and save virtualization checks.

## Not Implemented (By Design / Out of Scope for MO2 Plugin)

- Direct `A.EXE` patching at fixed offsets (startup stats, monster tables, wilderness lists, etc.).
- Runtime behavior patches noted in the reference (e.g. dungeon map unpack fix bytes).
- Full low-level editing workflows for encrypted `SAVEENGN.0x` beyond read-only metadata extraction.
- Writing or transforming `LOG.0x`/`SPELLS.0x` payload content.

## Future Updates (Post-1.0)

Items that are useful but not required for `1.0`:

- Deeper save parser parity with Daggerfall-level diagnostics:
  - Expand interpreted fields and developer details beyond current high-value offsets.
- Broader historical extension archaeology:
  - Add long-tail/rare file types only when confirmed by real mod packs.
- Advanced executable-level patch authoring support:
  - Exposed workflows for direct `A.EXE` offset patch sets (still safety-sensitive).
- Larger automated compatibility matrix:
  - Replay testing against expanded historical mod and save archives.

## Notes

- Offset documentation in the reference is for specific executable versions and compression states.
- Current plugin usage is read-oriented for mod management and save metadata display, not binary patch authoring.
- Technical reference highlights used for current prioritization:
  - `LOG.0x` structure and entry markers
  - `SAVEENGN.0x` encrypted offsets (level/exp/gold/blessing/detail/quest flags)
  - Canonical save bundle file set (`AUTOMAP`, `IN`, `LOG`, `SAVEENGN`, `SAVEGAME`, `SPELLS`, `STATES`, `WILDPAL`, `NAMES.DAT`)
