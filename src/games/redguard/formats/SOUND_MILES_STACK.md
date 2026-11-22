# Sound Driver Stack Files (Miles AIL Artifacts)

Last updated: 2026-03-08

## Scope
Documents non-`MAIN.SFX` sound-stack files found in `Redguard/sound`.

## Retail Corpus Observations
Directory: `Redguard/sound`

Representative extension counts (sampled install):
- `.mdi`: `20`
- `.dig`: `13`
- `.ad`: `2`
- `.opl`: `2`
- `.exe`: `4`
- `.INI`: `2`
- `.PIF`: `2`
- plus one-off support files (`.lst`, `.RST`, `.WAV`, `.SFX`)

## Signature Evidence
- `audiodrv.dig` starts with `AIL3DIG` signature (`41 49 4C 33 44 49 47 1A`).
- `adlib.mdi` starts with `AIL3MDI` signature (`41 49 4C 33 4D 44 49 1A`).
- `DIG.INI` / `MDI.INI` begin with text header referencing `Miles Sound System V4.0d`.
- `STATE.RST` is text-like state/config data (`[edit-]`, `screen=...`, etc.).
- `Fatman.ad` and `Fatman.opl` are same-sized binary tables in sampled install (`3622` bytes each).

## Interpretation Status
- `.dig` and `.mdi` are Miles Sound System driver/provider modules.
- `.ad`/`.opl` are associated instrument/patch data used by MIDI/OPL playback paths.
- `.INI`/`.lst`/`.RST` are support/configuration/state files for driver setup tooling.

## Boundary with Redguard Native Formats
- These files are middleware/backend assets and should be documented separately from native XnGine content containers (`SFX`, `RTX`, `TEXBSI`, etc.).
- Parser/writer work for Redguard gameplay assets should generally treat this stack as external dependency layer.

## Relationship to Existing Audio Docs
- `MAIN.SFX` container format remains documented in `SFX.md`.
- This file covers the **driver stack and playback backend assets**, not in-game effect container internals.

## Next Reverse-Engineering Targets
1. Inventory which executable/tool binary references each driver filename.
2. Identify whether `.dig/.mdi` files are selected dynamically via INI/device probing.
3. Confirm whether `.ad/.opl` are optional or mandatory for startup paths in this install flavor.

## Current Confidence
- High: signatures and directory role as external sound middleware assets.
- Low/Medium: deeper internal schema of `.dig/.mdi/.ad/.opl` binaries (out of current Redguard-format scope).
