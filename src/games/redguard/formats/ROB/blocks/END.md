# END - END

## Status
Confirmed.

## Purpose
4-byte ASCII footer marker at the end of a retail `ROB` file.

Confirmed behavior:
- After the final segment payload, all retail GOG `ROB` files observed in corpus
  terminate with:
  - `45 4E 44 20`
  - ASCII `END `

## Writer Notes
- Preserve exactly when writer support is added.
