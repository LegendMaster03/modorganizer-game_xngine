# KEYS.INI

Last updated: 2026-03-11

## Role
Input/key binding configuration.

Retail path:
- `Redguard/KEYS.INI` (`3034` bytes in current scan)

## Implementation Coverage
- Included in plugin `iniFiles()` set.
- No dedicated deep parser currently documented in workspace C++.

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `3034` bytes
- section count: `3`

Sections:
- `[input]`
- `[misc]`
- `[defined]`

### `[input]` key set (30 observed)
- action keys: `next_key`, `prev_key`, `inventory_key`, `log_key`, `quick_sword_key`, `quick_health_key`, `map_key`
- directional bindings: `key_up[0/1]`, `key_down[0/1]`, `key_left[0/1]`, `key_right[0/1]`
- action bindings: `key_a[0/1]`, `key_b[0/1]`, `key_c[0/1]`, `key_d[0/1]`
- user overrides: `user_key_up`, `user_key_left`, `user_key_right`, `user_key_a`, `user_key_b`, `user_key_c`, `user_key_d`

### `[misc]` key set
- `waiting_message` (observed value: `HIT KEY`)

### `[defined]` key set
- dense lookup map `key[0]..key[139]`
- additional `next_key` and `prev_key` labels present in this section
- values are key-name strings (examples: `escape`, `tab`, `enter`, `l shift`, alphabet keys)

## Value Shape Notes
- many bindings are integer keycodes in `[input]`.
- `[defined]` appears to provide keycode-to-label text mapping used by UI/editor prompts.

## Caveat
This page is currently inventory/shape documentation from retail scan; runtime code-path semantics for each keycode are not yet fully traced in workspace sources.
