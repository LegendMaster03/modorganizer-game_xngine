# RGM Enumeration Notes (Detailed)

Last updated: 2026-03-08

## AnimGroup Categories (External Reference)
The reference table lists IDs `0..168` and groups them by behavior category.

- Idles & Panic: `0..8`
- Misc (Low Priority): `9..13`
- Turns: `14..17`
- Simple Movements: `18..31`
- Falling & Jumping: `32..64`
- Land Groups: `65..74`
- Slides: `75..78`
- Ropes: `79..85`
- Ledges & Hand-Plants: `86..90`
- Combat: `91..134`
- Flask: `135..140`
- Swimming: `141..143`
- Special Movements: `144..151`
- Speech: `152..155`
- Misc (High Priority): `156..168`

Refer to `RGM_ENUMS.md` for the full explicit ID->name list.

## AnimType Notes (External Reference)
- `0`: NORMAL_GROUP / `animtype_stop`
  - Usually idle/panic style groups.
  - Reported to be immediately interruptible by transition to new group.
- `1`: COMBAT_GROUP / `animtype_no_stop`
  - Not only combat; includes groups that should exit gracefully/complete transitions.
- `2`: DEATH_GROUP / `animtype_no_panic`
  - Does not auto-fallback to panic when no input/action remains.

## FrameType Notes (External Reference)
- `0` ShowFrame: frame index points to RAAN frame.
- `1` EndAnimation: usually last frame marker.
- `2` GoToPrevious: reverse jump to previous frame record (loop/control logic).
- `3` GoToFuture: forward jump to future frame record (loop/control logic).
- `4` PlaySound: plays SFX from `SOUND/MAIN.SFX`; includes effect flag behavior.
- `5` BreakPoint: reported possible stop/checkpoint control use.
- `11` Rumble: used for screen-shake effects in limited cases.
- `15` Transition: transition control between groups (jump/run/landing style chains).

## ObjectType Notes (External Reference)
- `1` 3D Object
- `2` Audio object (script location for sound execution)
- `3` Flat (scripted flat textures)
- `4` Light Object (scripted visible light objects)
- `6` Light (RGB/shadow style; script `X` usage noted)
