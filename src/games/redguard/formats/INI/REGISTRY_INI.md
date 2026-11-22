# REGISTRY.INI

Last updated: 2026-03-11

## Role
Small registry/preferences style INI used by game/tooling.

Retail path:
- `Redguard/REGISTRY.INI` (`302` bytes in current scan)

## Implementation Coverage
- Included in plugin `iniFiles()` set.
- No dedicated deep parser currently documented in workspace C++.

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `302` bytes
- section count: `2`

Sections:
- `[registry]` (no assignments observed)
- `[file32]` (7 assignments observed)

### `[file32]` key set
- `use_registry`
- `use_32bit`
- `archive_file_first`
- `local_path`
- `ignore_registry_errors`
- `registry_log`
- `advanced_exist`

### Observed value shape
- boolean-style strings (`true`/`false`) for all observed keys.

Observed assignment snapshot:
- `use_registry = false`
- `use_32bit = true`
- `archive_file_first = false`
- `local_path = true`
- `ignore_registry_errors = false`
- `registry_log = false`
- `advanced_exist = true`

## Notes
- Despite filename, this is plain INI text and separate from host OS registry APIs.
- Workspace plugin does use Windows registry lookup for game detection, but that is code-path behavior (`findInRegistry`) and not parsed from this file.
