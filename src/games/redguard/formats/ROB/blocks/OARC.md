# OARC - OARC

## Status
Confirmed.

## Purpose
4-byte ASCII file signature at the start of a `ROB` file.

Confirmed position:
- `ROB+0x00..0x03` = `OARC`

Following fields:
- `ROB+0x04..0x07` = unknown `LE u32`
- `ROB+0x08..0x0B` = `NumSegments`

## Writer Notes
- Preserve exactly.
