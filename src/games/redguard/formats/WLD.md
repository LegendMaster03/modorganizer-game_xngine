# WLD (World Height/Texture Tile Map)

Last updated: 2026-03-11

## Scope
`*.WLD` files encode terrain height + texture tile indices and per-tile texture rotation for world maps.

This file format is used as a map surface source and paired with palette/texture assets selected by `WORLD.INI`.

## Header (Parser-Derived)
Unity parser reads a fixed-size header struct:
- `Unknown1[6]` (`s32[6]`)
- `SectionHeaderSize` (`s32`)
- `FileSize` (`s32`)
- `Unknown2[28]` (`s32[28]`)
- `SectionOffsets[4]` (`s32[4]`)
- `Unknown3[256]` (`s32[256]`)

Notes:
- Unity loader currently reads the 4 sections sequentially after header and does not actively seek using `SectionOffsets`.

## Section Model
A WLD file contains 4 quadrants (`sections[0..3]`).

Per-section struct (Unity parser):
- `Unknown1[3]` (`s16[3]`)
- `TexBsiFile` (`s16`)
- `Size` (`s16`) (observed as `256`)
- `Unknown2[6]` (`s16[6]`)
- `Map1` (`u8[(Size/2)*(Size/2)]`)
- `Map2` (`u8[(Size/2)*(Size/2)]`)
- `Map3` (`u8[(Size/2)*(Size/2)]`)
- `Map4` (`u8[(Size/2)*(Size/2)]`)

File-size profile from external WLD notes:
- header: `1184` bytes
- each section: `65558` bytes (`22`-byte section header + `4 * 16384` maps)
- footer: `16` bytes
- observed retail file size in reference notes: `263,432` bytes (`4` WLD files)

Interpretation status:
- The canonical parser model above remains implementation-grounded.
- Fixed-size/header/footer constants are currently tracked as externally documented observations and should be treated as high-confidence corpus notes until independently re-measured in local tooling.

## Reconstructed Full Maps
Unity combines the 4 quadrants into full-size maps (`Size x Size`):
- `HeightMap` from `Map1`
- `TextureMap` from `Map3`

Bitfield extraction:
- Height byte:
  - low 7 bits => height value (`0..127`)
  - high bit => extra flag
- Texture byte:
  - low 6 bits => texture index (`0..63`)
  - high 2 bits => UV rotation (`0..3`)

## Mesh Build Semantics (Unity Runtime)
The Unity loader generates terrain triangles with these observed rules:
- One tile cell => 2 triangles.
- Texture index chooses one of 64 submeshes/material slots.
- Rotation bits select one of 4 hard-coded UV orientation tables.
- Height is passed through a nonlinear height function during mesh generation.

Runtime scaling/constants used by this importer:
- X/Z tile scale: `12.9`
- world offsets: `(-12, 0, 11)`
- height scalar path uses importer polynomial:
  - `H(y) = 0.00000138917*y^4 - 0.000185238*y^3 + 0.0122758*y^2 + 0.450595*y + 1.92776`

These constants are importer/runtime tuning values and not guaranteed to be canonical game-engine constants.

Additional importer assumptions:
- mesh builder assumes fixed `64` terrain texture buckets (`texid_cnt = 64`) when partitioning submeshes.
- UV rotation triangles use hardcoded lookup tables with explicit comment indicating this avoids rounding artifacts.

## TexBsi Linkage
- `TexBsiFile` in section header is used by runtime to select the terrain `TEXBSI` set.
- Unity `RGMeshStore` currently uses section 0 `texbsi_file` when constructing world materials.

Mapping caveat from external reference notes:
- `Map3` low 6-bit texture IDs map to the numeric suffix in corresponding `TEXBSI` image names (e.g., `...59`) and may not match physical record order for animated images.

## Confidence
- High: section field sizes, 4-quadrant assembly, bit-split for height/texture/rotation.
- Medium: semantics of many unknown header/section fields and exact in-engine world-space scale constants.

## Source Provenance

External fixed-size WLD observations are corroborating evidence. The implementation-grounded layout and terrain/index behavior above remain authoritative.

## Sources
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGWLDFile.cs`
- `redguard-unity-master/redguard-unity-master/Assets/Scripts/RGFileImport/RGMeshStore.cs`