# FOG.INI

Last updated: 2026-03-11

## Role
Auxiliary INI found under `fxart` in retail install.

Retail path:
- `Redguard/fxart/FOG.INI` (`291` bytes in current scan)

## Implementation Coverage
- Not part of `GameRedguard::iniFiles()` core list.
- No dedicated parser documented in workspace code path yet.

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `291` bytes
- line count: `21`

Observed shape:
- no INI sections (`[section]`) and no `key=value` assignments
- header comments describe table semantics
- data lines are comma-separated tuples, followed by a terminal single value

Raw content pattern:
- comments:
	- `;index,value.  63 is maximum index, 255 is max number.`
	- `;fills from 1 index to next with current number.`
	- `;46 is the final value for 3800 render distance`
- table rows (examples):
	- `0,0,`
	- `33,1,`
	- `46,136,`
	- `48,255,`
- terminal line:
	- `63`

## Interpretation (preliminary)
- file appears to be a compact index/value ramp table (likely fog lookup/curve data), not a conventional INI map.
- sectionless CSV-like format explains why generic INI section/key scanners report zero sections.

## Caveat
- runtime consumer for this file is not yet traced in workspace code; semantics above are structural observations from retail file content.
