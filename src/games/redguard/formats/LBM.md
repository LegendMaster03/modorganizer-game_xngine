# LBM (Interchange Bitmap Family)

Last updated: 2026-03-11

## Supported Blocks
- `FORM`

## Scope
Redguard ships `*.LBM` assets in system/UI paths. Current state is now chunk-level documented for `GUI.LBM` with PBM-style IFF framing.

## Retail GOG Validation (Install Corpus)
Validated against:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/system`

Observed:
- `GUI.LBM` present in retail `system/` alongside `GUI.GXA`, `.ANM`, and related UI resources.

## Container Notes
- `LBM` files are IFF-family containers and begin with `FORM` marker framing.
- `GUI.LBM` validates as `FORM` kind `PBM`.
- In Redguard context, these appear to coexist with GXA/PCX/BMP UI resources rather than replace them universally.

Representative `GUI.LBM` structure (top-level chunks):
- `BMHD` (`20`)
- `CMAP` (`768`)
- `DPPS` (`110`)
- multiple `CRNG` chunks (`8` each)
- `TINY` (`3407`)
- `BODY` (`195,218`)

BMHD decoded values (`GUI.LBM`):
- width/height: `640 x 480`
- x/y origin: `0 / 0`
- planes: `8`
- compression: `1` (RLE-compressed body path in PBM conventions)
- transparent index: `0`
- aspect: `1:1`
- page size: `640 x 480`

## Parsing Guidance
- Treat file as IFF `FORM` container and enumerate child chunk IDs + sizes.
- Handle even-byte alignment/padding between chunks (`len + (len % 2)`).
- Preserve unknown chunks verbatim for round-trip safety.
- Prefer tolerant parsing of optional chunks (`DPPS`, `CRNG`, `TINY`) before `BODY`.

## Open Questions
- Whether non-`GUI.LBM` Redguard LBM assets use the same chunk set/order.
- Exact semantics of `DPPS`, `CRNG`, and `TINY` in Redguard runtime usage.
- Runtime preference order between `.LBM` and `.GXA` assets when both exist.

## Confidence
- High: IFF/PBM framing, BMHD core fields, inline palette/body presence in `GUI.LBM`.
- Medium: chunk-order assumptions beyond `GUI.LBM` and optional-chunk behavior.
- Low: runtime-specific semantics for non-core chunks.

## Sources
- Retail corpus: `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/system/GUI.LBM`
- Repository reverse-engineering notes in `src/games/redguard/formats/`
