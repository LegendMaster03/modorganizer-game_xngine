# MDI.INI

Last updated: 2026-03-11

## Role
Miles sound stack configuration INI under `sound`.

Retail path:
- `Redguard/sound/MDI.INI` (`197` bytes in current scan)

## Implementation Coverage
- Not part of `GameRedguard::iniFiles()` core list.
- Covered contextually in sound stack notes.

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `197` bytes
- line count: `10`

Observed shape:
- no `[section]` headers
- no `key=value` pairs
- comment banner + whitespace-delimited `KEY VALUE` records

Header comment block:
- `;`
- `;Miles Sound System V4.0d of 30-Mar-98`
- `;`

Driver profile records:
- `DEVICE      Creative Labs Sound Blaster or 100% compatible`
- `DRIVER      SBLASTER.MDI`
- `IO_ADDR     220h`
- `IRQ         7`
- `DMA_8_BIT   1`
- `DMA_16_BIT  5`

## Interpretation
- This file appears to configure Miles MIDI/device driver selection and hardware port/IRQ/DMA parameters.
- As with `DIG.INI`, parser paths assuming `=` delimiters need custom handling for this format.

## Related Docs
- `../SOUND_MILES_STACK.md`
