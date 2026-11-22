# Suspect Save Pair Diffs (Non-`SAVEGAME.SAV`)

Last updated: 2026-03-08

## Scope
Pairwise diff of suspect "broken then fixed" slots, excluding `SAVEGAME.SAV`.
Compared pairs:
- `SAVEGAME.001 -> SAVEGAME.002`
- `SAVEGAME.009 -> SAVEGAME.010`
- `SAVEGAME.025 -> SAVEGAME.026`

Corpus path:
- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard/SAVEGAME`

## Results

### `SAVEGAME.001 -> SAVEGAME.002`
- `A_files=19`, `B_files=2`
- `Common=2`, `SameHash=1`, `DiffHash=1`
- `OnlyInA=17`, `OnlyInB=0`

Only in A:
- `BELLTOWR.TSG`
- `BRENNANS.TSG`
- `CARTOGR.TSG`
- `CATACOMB.TSG`
- `GERRICKS.TSG`
- `HARBTOWR.TSG`
- `ISLAND.TSG`
- `JFFERS.TSG`
- `MGUILD.TSG`
- `NECRISLE.TSG`
- `PALACE.TSG`
- `ROLLOS.TSG`
- `SILVER1.TSG`
- `SILVER2.TSG`
- `SMDEN.TSG`
- `TAVERN.TSG`
- `TEMPLE.TSG`

Changed in both:
- `LOGBOOK.TXT` (`11672 -> 553` bytes)

### `SAVEGAME.009 -> SAVEGAME.010`
- `A_files=27`, `B_files=16`
- `Common=16`, `SameHash=3`, `DiffHash=13`
- `OnlyInA=11`, `OnlyInB=0`

Only in A:
- `BRENNANS.TSG`
- `CAVERNS.TSG`
- `DRINT.TSG`
- `EXTPALAC.TSG`
- `HIDEINT.TSG`
- `JAILINT.TSG`
- `NECRTOWR.TSG`
- `OBSERVE.TSG`
- `ROLLOS.TSG`
- `SMDEN.TSG`
- `VILE.TSG`

Changed in both (size view):
- `BELLTOWR.TSG` (`60881 -> 60671`)
- `CARTOGR.TSG` (`66201 -> 66201`) *(hash differs, same size)*
- `CATACOMB.TSG` (`591587 -> 590963`)
- `GERRICKS.TSG` (`72056 -> 72056`) *(hash differs, same size)*
- `HARBTOWR.TSG` (`63266 -> 63296`)
- `ISLAND.TSG` (`977878 -> 976384`)
- `JFFERS.TSG` (`59538 -> 59628`)
- `LOGBOOK.TXT` (`18307 -> 7935`)
- `MGUILD.TSG` (`85914 -> 85794`)
- `NECRISLE.TSG` (`304542 -> 304780`)
- `SILVER1.TSG` (`63899 -> 63899`) *(hash differs, same size)*
- `TAVERN.TSG` (`79808 -> 79688`)
- `TEMPLE.TSG` (`75559 -> 75499`)

### `SAVEGAME.025 -> SAVEGAME.026`
- `A_files=27`, `B_files=23`
- `Common=23`, `SameHash=11`, `DiffHash=12`
- `OnlyInA=4`, `OnlyInB=0`

Only in A:
- `EXTPALAC.TSG`
- `JAILINT.TSG`
- `NECRTOWR.TSG`
- `VILE.TSG`

Changed in both (size view):
- `CARTOGR.TSG` (`66201 -> 66201`) *(hash differs, same size)*
- `CATACOMB.TSG` (`591587 -> 590963`)
- `CAVERNS.TSG` (`496309 -> 496039`)
- `GERRICKS.TSG` (`72056 -> 72056`) *(hash differs, same size)*
- `ISLAND.TSG` (`977878 -> 977966`)
- `JFFERS.TSG` (`59538 -> 59538`) *(hash differs, same size)*
- `LOGBOOK.TXT` (`18307 -> 15353`)
- `MGUILD.TSG` (`85914 -> 85544`)
- `NECRISLE.TSG` (`304542 -> 304780`)
- `SMDEN.TSG` (`66610 -> 66610`) *(hash differs, same size)*
- `TAVERN.TSG` (`79808 -> 79808`) *(hash differs, same size)*
- `TEMPLE.TSG` (`75559 -> 75559`) *(hash differs, same size)*

## Interpretation
- Differences are **not limited** to missing/present `SAVEGAME.SAV`.
- The paired slots also differ in:
  - included `TSG` set (often strict subset on the later slot),
  - content of overlapping `TSG` files,
  - `LOGBOOK.TXT` content/size.
- This supports a "broken/intermediate then revised/fixed" transition model more strongly than a pure metadata-file omission model.
