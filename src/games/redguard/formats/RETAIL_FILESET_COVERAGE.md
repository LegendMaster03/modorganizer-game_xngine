# Retail Fileset Coverage (Redguard)

Last updated: 2026-03-08

## Purpose
Tracks documentation coverage for file families observed in retail Redguard install.

## Coverage Snapshot
Core game-data formats (documented):
- `RGM`, `ROB`, `3D/3DC`, `WLD`, `PVO`
- `TEXBSI`/`BSI`, `COL`, `GXA`, `FNT`, `LBM`
- `RTX`, `SFX`
- `INI` families
- `SAV/TSG` save corpus behavior

Newly documented support families:
- `SWG/SWX` → `SWG_SWX.md`
- `SOUP386.DEF` → `SOUP386_DEF.md`
- Sound middleware/driver stack (`.dig/.mdi/.ad/.opl` + support files) → `SOUND_MILES_STACK.md`

Operational inventory artifacts:
- Full TSG log: `TSG_FILES_INDEX.csv`
- TSG summary: `TSG_FILES_INDEX.md`

## Remaining Gaps (As of this snapshot)
- Deep binary schema decode for:
  - `SWG/SWX`
  - Miles driver modules (`.dig/.mdi` internals)
  - instrument tables (`.ad/.opl` internals)
- Executable-driven integration behavior (`REDGUARD.EXE`, `RGFX.EXE`, setup tools) remains only partially mapped.
- Full canonical semantic decode for many `SAVEGAME.SAV` chunk tags (`SM*`, `SV*`) remains pending.

## Notes
- Numbered `TEXBSI.###` files in `fxart` are already treated as sparse-shard corpus and covered under `TEXBSI.md`.
- Save corpus currently shows both complete slots (`SAVEGAME.SAV` + `TSG`) and incomplete slots (`TSG` without `SAVEGAME.SAV`), but latest pairwise evidence favors an **intermediate/broken then fixed** interpretation for missing-`SAV` cases.
- See `SAV/SAV_TSG.md` and `SAV/SAVE_PAIR_DIFFS.md` for the current conservative save model.

## GOG vs Original fileset differences (provenance)
The following lists were provided in the Daveh / UESP notes and record files observed in GOG and original retail variants. They are retained here for provenance and corpus coverage purposes.

Files only present in the GOG release (excerpt):
3DFXCYRS.3DC
3dinfo.txt
4TODD001.3DC
CATA.COL
CATACOMB.COL
CAVETEST.COL
CLDTEST.COL
CLOTH.COL
COINS.COL
CRAK0000.3D
CRAK0000.3DC
CRAK0001.3DC
CRAK0002.3DC
CRAK0003.3DC
CRAK0004.3DC
CRAK0005.3DC
CRAK0006.3DC
CRAK0007.3DC
CRAK0008.3DC
CRAK0009.3DC
CRAK0010.3DC
CRAK0011.3DC
CRAK0012.3DC
CRAK0013.3DC
CRAK0014.3DC
CRAK0015.3DC
CRAK0016.3DC
CRAK0017.3DC
CRAK0018.3DC
CRAK0019.3DC
CRAK0020.3DC
CRAK0021.3DC
CRAK0022.3DC
CRAK0023.3DC
CRAK0024.3D
CRAK0024.3DC
CV_BOOM.3DC
CV_TRAP1.3DC
CV_TRAP2.3DC
CV_TRAP3.3DC
CV_TRAP4.3DC
CV_TRAP5.3DC
CV_TRAP6.3DC
CYRSTEST.3DC
CYRUSA01.3DC
FLAG_RL.3DC
FOG.INI
fxart.txt
fxart3dc.txt
GARDSDED.3DC
HIDEOUT2.COL
HUGH.COL
ISLAND.COL
ISLE3DFX.COL
LHBM0.3D
LHBM0.3DC
LHBM0000.3DC
LHBM1.3D
LHBM1.3DC
LHBM2.3DC
LHBM3.3D
LHBM3.3DC
LHBM4.3D
LHBM4.3DC
LOGFILE.TXT
MBPGA001.3DC
MBPGA002.3DC
MBPGA003.3DC
MENUA001.3DC
MGUILD2.COL
MKCRATE.3D
MKCRATE1.3D
more
NECRO.COL
NECRO2.COL
OBSERVAT.COL
PALACE00.COL
REDCAVE.COL
REDEDIT.COL
REDGUARD.COL
SCR3D00.3DC
SCR3D01.3DC
SCR3D02.3DC
SCR3D03.3DC
SCR3D1.3DC
SKELETON.3DC
SKY.COL
SKY001.BSI
SKY888.BSI
SKY888.COL
SKY899.BSI
SKY899.COL
SKYNEC.BSI
SKYNEC.COL
SKYNIT.BSI
SKYNIT.COL
sun001.bsi
TAVERN.COL
TROLA000.3DC
TROLL.3DC
TV_BANNR.3D
TV_BAR.3D
TV_BARRL.3D
TV_BMUGG.3D
TV_BOTTL.3DC
XWANTED.3D

Files only present in the Original (?) release (excerpt):
art_pal.col
cata.col
catacomb.col
catatest.col
CRAIGTST.ROB
CYRSTEST.ROB
DIALTEST.ROB
HAZE.000
HAZE.001
HAZE.002
HAZE.003
HAZE.004
HAZE.005
HAZE.006
HAZE.010
HAZE.017
HAZE.028
HAZE.666
HAZE.961
HAZE.999
HAZE_OLD.001
HOLETEST.ROB
island.col
KURT2.ROB
light.dat
MKTEST.ROB
necro.col
necro2.col
observat.col
palace00.col
redcave.col
rededit.col
redguard.col
SEANTEST.ROB
SHADE.000
SHADE.001
SHADE.002
SHADE.003
SHADE.004
SHADE.005
SHADE.006
SHADE.010
SHADE.013
SHADE.017
SHADE.028
SHADE.600
SHADE.666
SHADE.999
sky.col
sky61.col
STROSMKI.ROB
TEX.TXT
TODDCATA.ROB
TRAVIS.ROB

Small provenance artifacts mentioned in the notes (not all currently present in repository):
- `3dinfo.txt`, `fxart.txt`, `fxart3dc.txt`, `More` (generated lists), and `LOGFILE.TXT` (collision-sphere generation log).

These lists are retained for corpus provenance; they may be pruned or moved into a separate `provenance/` file if preferred.
