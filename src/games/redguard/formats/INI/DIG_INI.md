# DIG.INI

Last updated: 2026-03-11

## Role
Miles sound stack configuration INI under `sound`.

Retail path:
- `Redguard/sound/DIG.INI` (`192` bytes in current scan)

## Implementation Coverage
- Not part of `GameRedguard::iniFiles()` core list.
- Covered contextually in sound stack notes.

## Retail Structure Snapshot (Current Install)

File metrics:
- size: `192` bytes
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
- `DEVICE      Creative Labs SoundBlaster 16, AWE32 or AWE64`
- `DRIVER      SB16.DIG`
- `IO_ADDR     220h`
- `IRQ         7`
- `DMA_8_BIT   1`
- `DMA_16_BIT  5`

## Interpretation
- This file behaves more like a small device-profile config used by Miles driver bootstrap than a general game INI section map.
- Generic INI parsers expecting `=` separators will not parse these lines as key/value without custom tokenization.

## Related Docs
- `../SOUND_MILES_STACK.md`
