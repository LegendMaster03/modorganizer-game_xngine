# OARD - OARD

## Status
Confirmed.

## Purpose
Second 4-byte ASCII header marker in `ROB`.

Confirmed position:
- `ROB+0x0C..0x0F` = `OARD`

Following field:
- `ROB+0x10..0x13` = unknown `LE u32`

This confirms the full retail `ROB` header is 20 bytes, not 36 bytes.

## Writer Notes
- Preserve exactly.
