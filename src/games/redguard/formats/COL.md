# COL (Palette File)

Last updated: 2026-03-11

## Scope
`*.COL` files provide external 256-color palettes used by TEXBSI images when inline `CMAP` is absent.

## Layout
1. `FileSize` (`u32`)
2. `Unknown_04` (`u32`)
3. `PaletteData` (`u8[FileSize - 8]`)

Palette interpretation:
- Data is read as RGB triplets (`R,G,B`).
- Canonical expected size is `256 * 3 = 768` bytes.
- Java texture tooling consumes COL palette bytes without `*4` scaling (unlike FNT/GXA palette paths in that project).

## Behavior Observed in Reference Parser
- If palette bytes are fewer than 768, parser warns and reads available colors only.
- If palette bytes are greater than 768, parser warns and reads first 768 bytes.
- Palette array is normalized to 256 entries by zero-filling missing colors.

Unity importer path note (`RGTexStore`):
- Palette keying is uppercase (`palname`) for cache lookups, but one explicit filename special-case exists:
	- palette name `SUNSET` resolves to lowercase on disk (`sunset.col`) in Unity importer code.
- Practical implication: case-normalization and one known exception are handled in tooling layer, not inferred from COL payload bytes.

## Byte Order
- Header values are read directly as host-order `dword` in the C++ reference parser (Windows x86/x64 usage implies little-endian in practice).

## Source Provenance

External UESP-style layout notes are reviewed as corroborating evidence. The canonical PAL payload model and tooling caveats above take precedence.

## Sources
- `redguard-3dfiletest-master/3DFileTest/Common/RedguardTexBsiFile.h`
- `redguard-3dfiletest-master/3DFileTest/Common/RedguardTexBsiFile.cpp`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGTexStore.cs`
