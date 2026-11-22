# SVIT -SaVe ITem

Last updated: 2026-03-28

## Status
Mostly mapped for `SAVEGAME.SAV`.

`SVIT` is now confirmed as the main item-system chunk in `SAVEGAME.SAV`, with a stable
per-item record table. The same tag also appears in many `*.TSG` files. Current corpus evidence
shows the `.TSG` variant has the **same payload length and field layout**, but not the same
semantic role as the global `SAVEGAME.SAV` inventory snapshot.

## Purpose
Stores serialized item totals, per-item player limits, and current player-held counts for the
global Redguard item registry.

## Chunk Framing / Selection
- Chunk framing is the standard save layout used elsewhere in `SAVEGAME.SAV`:
  - `Tag[4]`
  - `Length[4]` (`BE u32`)
  - `Payload[Length]`
- Runtime/tooling convention remains: use the **last bounded-valid** `SVIT` chunk in the file.
- In confirmed `SAVEGAME.SAV` samples, payload length is `0x768` (`1896`) bytes.
- This supersedes the earlier `104`-byte interpretation for `SAVEGAME.SAV`; that earlier model was
  not the canonical full payload layout.

## Relationship To `ITEM.INI`
- Item record index `n` in `SVIT` uses the same canonical item ID as indexed keys in `ITEM.INI`.
- Confirmed correspondences:
  - `SVIT.total[n]` <-> `ITEM.INI total[n]`
  - `SVIT.player_max[n]` <-> `ITEM.INI player_max[n]`
  - `SVIT.current_count[n]` = serialized live player inventory count for item `n`
- This is the same ID space used by script functions such as `HaveItem`, `AddItem`, `DropItem`,
  `SelectItem`, and related item operations.

## Confirmed `SAVEGAME.SAV` Payload Layout

### High-level structure
- Item records cover item IDs `0..85` inclusive.
- Record size is `0x16` (`22`) bytes.
- Record base formula:
  - `record_base(n) = n * 0x16`
- Payload ends with a trailing `LE u32 = 1` at payload offset `0x764`.
- Item ID `86` (`LAST` in the retail `ITEM.INI`) does **not** appear as a normal `0x16`-byte
  record in the confirmed layout.

### Per-record layout
For item ID `n`, at `base = n * 0x16`:

| Payload Offset | Type | Meaning |
|---|---|---|
| `base + 0x00` | `u16` | Unknown / reserved |
| `base + 0x02` | `LE u32` | `total[n]` |
| `base + 0x06` | `LE u32` | `player_max[n]` |
| `base + 0x0A` | `LE u32` | `current_count[n]` |
| `base + 0x0E` | `byte[8]` | Unknown / reserved |

Observed behavior in the controlled probe save:
- The `u16` prefix at `base + 0x00` remained `0`.
- The `byte[8]` tail at `base + 0x0E` remained all zeroes.
- The three `LE u32` fields changed exactly as expected from the forced `ITEM.INI` values.

## Confirmed Offset Formulae
- `total[n]` payload offset = `n * 0x16 + 0x02`
- `player_max[n]` payload offset = `n * 0x16 + 0x06`
- `current_count[n]` payload offset = `n * 0x16 + 0x0A`

### Complete item mapping table
| ID | Item | Record Base | `total` | `player_max` | `current_count` |
|---:|---|---:|---:|---:|---:|
| `0` | `COMPASS` | `0x000` | `0x002` | `0x006` | `0x00A` |
| `1` | `SABRE` | `0x016` | `0x018` | `0x01C` | `0x020` |
| `2` | `GOLD` | `0x02C` | `0x02E` | `0x032` | `0x036` |
| `3` | `POTION OF IRONSKIN` | `0x042` | `0x044` | `0x048` | `0x04C` |
| `4` | `HEALTH POTION` | `0x058` | `0x05A` | `0x05E` | `0x062` |
| `5` | `RING OF INVISIBILITY` | `0x06E` | `0x070` | `0x074` | `0x078` |
| `6` | `VOA'S RING` | `0x084` | `0x086` | `0x08A` | `0x08E` |
| `7` | `GUARD SWORD` | `0x09A` | `0x09C` | `0x0A0` | `0x0A4` |
| `8` | `RUSTY KEY` | `0x0B0` | `0x0B2` | `0x0B6` | `0x0BA` |
| `9` | `GOLD KEY` | `0x0C6` | `0x0C8` | `0x0CC` | `0x0D0` |
| `10` | `SILVER KEY` | `0x0DC` | `0x0DE` | `0x0E2` | `0x0E6` |
| `11` | `AMULET` | `0x0F2` | `0x0F4` | `0x0F8` | `0x0FC` |
| `12` | `SOUL GEM` | `0x108` | `0x10A` | `0x10E` | `0x112` |
| `13` | `SOUL SWORD` | `0x11E` | `0x120` | `0x124` | `0x128` |
| `14` | `CROW BAR` | `0x134` | `0x136` | `0x13A` | `0x13E` |
| `15` | `RUNE (2 LINES AND A DOT)` | `0x14A` | `0x14C` | `0x150` | `0x154` |
| `16` | `RUNE (2 LINES)` | `0x160` | `0x162` | `0x166` | `0x16A` |
| `17` | `RUNE (A LINE AND DOT)` | `0x176` | `0x178` | `0x17C` | `0x180` |
| `18` | `LETTER` | `0x18C` | `0x18E` | `0x192` | `0x196` |
| `19` | `ORC'S BLOOD` | `0x1A2` | `0x1A4` | `0x1A8` | `0x1AC` |
| `20` | `ORC'S BLOOD (SUBLIMATED)` | `0x1B8` | `0x1BA` | `0x1BE` | `0x1C2` |
| `21` | `SPIDER'S MILK` | `0x1CE` | `0x1D0` | `0x1D4` | `0x1D8` |
| `22` | `SPIDER'S MILK (SUBLIMATED)` | `0x1E4` | `0x1E6` | `0x1EA` | `0x1EE` |
| `23` | `ECTOPLASM` | `0x1FA` | `0x1FC` | `0x200` | `0x204` |
| `24` | `ECTOPLASM (SUBLIMATED)` | `0x210` | `0x212` | `0x216` | `0x21A` |
| `25` | `HIST SAP` | `0x226` | `0x228` | `0x22C` | `0x230` |
| `26` | `HIST SAP (SUBLIMATED)` | `0x23C` | `0x23E` | `0x242` | `0x246` |
| `27` | `'DWARVEN LORE'` | `0x252` | `0x254` | `0x258` | `0x25C` |
| `28` | `DWARVEN GEAR` | `0x268` | `0x26A` | `0x26E` | `0x272` |
| `29` | `GLASS VIAL` | `0x27E` | `0x280` | `0x284` | `0x288` |
| `30` | `GLASS VIAL (WITH ELIXIR)` | `0x294` | `0x296` | `0x29A` | `0x29E` |
| `31` | `IRON WEIGHT` | `0x2AA` | `0x2AC` | `0x2B0` | `0x2B4` |
| `32` | `BUCKET` | `0x2C0` | `0x2C2` | `0x2C6` | `0x2CA` |
| `33` | `BUCKET WITH WATER` | `0x2D6` | `0x2D8` | `0x2DC` | `0x2E0` |
| `34` | `RUNE (fist)` | `0x2EC` | `0x2EE` | `0x2F2` | `0x2F6` |
| `35` | `'ELVEN ARTIFACTS VIII' (COPY)` | `0x302` | `0x304` | `0x308` | `0x30C` |
| `36` | `'ELVEN ARTIFACTS VIII'` | `0x318` | `0x31A` | `0x31E` | `0x322` |
| `37` | `'REDGUARD HISTORY'` | `0x32E` | `0x330` | `0x334` | `0x338` |
| `38` | `'FLORA OF HAMMERFELL'` | `0x344` | `0x346` | `0x34A` | `0x34E` |
| `39` | `MAP` | `0x35A` | `0x35C` | `0x360` | `0x364` |
| `40` | `LEATHER POUCH` | `0x370` | `0x372` | `0x376` | `0x37A` |
| `41` | `Trithik's MAP PIECE` | `0x386` | `0x388` | `0x38C` | `0x390` |
| `42` | `SILVER BOAT` | `0x39C` | `0x39E` | `0x3A2` | `0x3A6` |
| `43` | `SHOVEL` | `0x3B2` | `0x3B4` | `0x3B8` | `0x3BC` |
| `44` | `ALOE` | `0x3C8` | `0x3CA` | `0x3CE` | `0x3D2` |
| `45` | `TORCH` | `0x3DE` | `0x3E0` | `0x3E4` | `0x3E8` |
| `46` | `MONOCLE` | `0x3F4` | `0x3F6` | `0x3FA` | `0x3FE` |
| `47` | `FLAG` | `0x40A` | `0x40C` | `0x410` | `0x414` |
| `48` | `SILVER LOCKET` | `0x420` | `0x422` | `0x426` | `0x42A` |
| `49` | `LEAGUE INSIGNIA` | `0x436` | `0x438` | `0x43C` | `0x440` |
| `50` | `JOTO'S MAP PIECE` | `0x44C` | `0x44E` | `0x452` | `0x456` |
| `51` | `FLASK OF LILLANDRIL` | `0x462` | `0x464` | `0x468` | `0x46C` |
| `52` | `TALISMAN` | `0x478` | `0x47A` | `0x47E` | `0x482` |
| `53` | `ISZARA'S JOURNAL (OPEN)` | `0x48E` | `0x490` | `0x494` | `0x498` |
| `54` | `CANAH FEATHER` | `0x4A4` | `0x4A6` | `0x4AA` | `0x4AE` |
| `55` | `KITHRAL'S JOURNAL` | `0x4BA` | `0x4BC` | `0x4C0` | `0x4C4` |
| `56` | `'FFOULKES FIRMAMENT'` | `0x4D0` | `0x4D2` | `0x4D6` | `0x4DA` |
| `57` | `ISZARA'S JOURNAL (LOCKED)` | `0x4E6` | `0x4E8` | `0x4EC` | `0x4F0` |
| `58` | `STARSTONE` | `0x4FC` | `0x4FE` | `0x502` | `0x506` |
| `59` | `KEY TO KRISANDRA'S STOREROOM` | `0x512` | `0x514` | `0x518` | `0x51C` |
| `60` | `KEY TO ISZARA'S LODGE` | `0x528` | `0x52A` | `0x52E` | `0x532` |
| `61` | `N'GASTA'S NECROMANCY BOOK` | `0x53E` | `0x540` | `0x544` | `0x548` |
| `62` | `BAR MUG` | `0x554` | `0x556` | `0x55A` | `0x55E` |
| `63` | `MARIAH'S WATERING CAN` | `0x56A` | `0x56C` | `0x570` | `0x574` |
| `64` | `GLASS BOTTLE` | `0x580` | `0x582` | `0x586` | `0x58A` |
| `65` | `GLASS BOTTLE WITH PURE WATER` | `0x596` | `0x598` | `0x59C` | `0x5A0` |
| `66` | `PURE WATER AND ALOE MIX` | `0x5AC` | `0x5AE` | `0x5B2` | `0x5B6` |
| `67` | `STRENGTH POTION` | `0x5C2` | `0x5C4` | `0x5C8` | `0x5CC` |
| `68` | `BANDAGE` | `0x5D8` | `0x5DA` | `0x5DE` | `0x5E2` |
| `69` | `BLOODY BANDAGE` | `0x5EE` | `0x5F0` | `0x5F4` | `0x5F8` |
| `70` | `SKELETON SWORD` | `0x604` | `0x606` | `0x60A` | `0x60E` |
| `71` | `KEEP OUT` | `0x61A` | `0x61C` | `0x620` | `0x624` |
| `72` | `NO TRESPASSING` | `0x630` | `0x632` | `0x636` | `0x63A` |
| `73` | `TOBIAS' BAR MUG` | `0x646` | `0x648` | `0x64C` | `0x650` |
| `74` | `BONE KEY` | `0x65C` | `0x65E` | `0x662` | `0x666` |
| `75` | `FLAMING SABRE` | `0x672` | `0x674` | `0x678` | `0x67C` |
| `76` | `GOBLIN SWORD` | `0x688` | `0x68A` | `0x68E` | `0x692` |
| `77` | `OGRE'S AXE` | `0x69E` | `0x6A0` | `0x6A4` | `0x6A8` |
| `78` | `DRAM'S SWORD` | `0x6B4` | `0x6B6` | `0x6BA` | `0x6BE` |
| `79` | `SILVER KEY (PALACE)` | `0x6CA` | `0x6CC` | `0x6D0` | `0x6D4` |
| `80` | `DRAM'S BOW` | `0x6E0` | `0x6E2` | `0x6E6` | `0x6EA` |
| `81` | `DRAM'S ARROW` | `0x6F6` | `0x6F8` | `0x6FC` | `0x700` |
| `82` | `SILVER LOCKET (COPY)` | `0x70C` | `0x70E` | `0x712` | `0x716` |
| `83` | `ISLAND MAP` | `0x722` | `0x724` | `0x728` | `0x72C` |
| `84` | `WANTED POSTER` | `0x738` | `0x73A` | `0x73E` | `0x742` |
| `85` | `PALACE DIAGRAM` | `0x74E` | `0x750` | `0x754` | `0x758` |
| `86` | `LAST` | n/a | n/a | n/a | n/a |

## Known User-Facing Mappings
The previously known scalar offsets are now explained as ordinary `current_count[n]` fields:
- `SVIT+0x036` = item `2` current count = `GOLD`
- `SVIT+0x04C` = item `3` current count = `POTION OF IRONSKIN`
- `SVIT+0x062` = item `4` current count = `HEALTH POTION`
- `SVIT+0x5CC` = item `67` current count = `STRENGTH POTION`

## Controlled Validation
A deliberate probe save was created by forcing the live MO2 override `ITEM.INI` values so that,
for each item ID `n`:
- `total[n] = n + 0x71`
- `player_max[n] = n + 0x71`
- `player_total[n] = n + 0x71`

Then a fresh save was created at:
- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard/SAVEGAME/SAVEGAME.003/SAVEGAME.SAV`

Results:
- For item IDs `0..85`, `SVIT.total[n]`, `SVIT.player_max[n]`, and `SVIT.current_count[n]`
  matched the forced `n + 0x71` pattern exactly.
- This confirms both:
  - the record stride (`0x16`),
  - and the identity of the three `LE u32` fields.

Representative probe-save rows:
- Full per-item mapping is listed above for every item ID `0..85`.
- Sample probe-save values:
  - Item `0` (`COMPASS`): `total=113`, `player_max=113`, `current=113`
  - Item `67` (`STRENGTH POTION`): `total=180`, `player_max=180`, `current=180`
  - Item `85` (`PALACE DIAGRAM`): `total=198`, `player_max=198`, `current=198`

## Retail Save Cross-check
Retail sample:
- `C:/Program Files (x86)/GOG Galaxy/Games/Redguard/Redguard/SAVEGAME/SAVEGAME.001/SAVEGAME.SAV`

Confirmed examples from the stock save:
- Item `1` (`SABRE`): `total=1`, `player_max=1`, `current=1`
- Item `2` (`GOLD`): `total=1000000`, `player_max=500`, `current=53`
- Item `3` (`POTION OF IRONSKIN`): `total=10000`, `player_max=10000`, `current=0`
- Item `4` (`HEALTH POTION`): `total=10000`, `player_max=10000`, `current=6`
- Item `67` (`STRENGTH POTION`): `total=10000`, `player_max=10000`, `current=0`

This confirms the third field is not merely a copy of `player_total[n]`; it is the live serialized
inventory count at save time.

## External Editor Corroboration (`REDEDIT`)
- Legacy VB6 editor `REDEDIT` contains the literal string `SVIT`.
- The editor exposes item-related edits for:
  - Gold
  - Health Potions
  - Potion of Ironskin
  - Strength Potions
- These all now map cleanly into the confirmed `current_count[n]` fields above.

Previously observed REDEDIT candidate constants:
- `0x003C`
- `0x0052`
- `0x0068`
- `0x05D2`

Interpretation remains unresolved:
- They do **not** match the canonical payload offsets directly
  (`0x036`, `0x04C`, `0x062`, `0x5CC`).
- They may represent absolute positions in another coordinate system, transformed offsets, or
  positions in an intermediate hex/text buffer used by the tool.

## TSG Caveat
- `SVIT` also appears in many `*.TSG` files.
- Current corpus scan result:
  - all scanned `SAVEGAME.SAV` `SVIT` payloads: `0x768` (`1896`) bytes
  - all scanned `.TSG` `SVIT` payloads: `0x768` (`1896`) bytes
- This strongly suggests the same binary table layout is reused across both file families.
- However, `.TSG` `SVIT` should still **not** be interpreted with the same semantic meaning as
  the player/global inventory snapshot in `SAVEGAME.SAV`.

### What the corpus supports
- The three main per-item fields (`total[n]`, `player_max[n]`, `current_count[n]`) appear at the
  same offsets in both `SAVEGAME.SAV` and `.TSG`.
- In sampled saves, `total[n]` and `player_max[n]` are usually identical across `SAVEGAME.SAV`
  and sibling `.TSG` files.
- `current_count[n]` often differs:
  - between `SAVEGAME.SAV` and `.TSG`
  - and between sibling `.TSG` files in the same slot
- This makes the simple “`.TSG` `SVIT` is just the room item list” theory unlikely.
- The stronger current model is:
  - same binary item table
  - different save-state layer / area-scoped snapshot semantics

### What the corpus does not yet prove
- Whether `.TSG` `SVIT` is a true override, a cached area-local snapshot, or some other
  companion state layer.
- Which parts of `.TSG` `SVIT` the engine actually consults when loading the active area.
- How `.TSG` `SVIT` interacts with `SAVEGAME.SAV` `SVIT` when the same item counts differ.

## Unknowns
- Meaning of the per-record `u16` at `base + 0x00`.
- Meaning of the per-record trailing `byte[8]` at `base + 0x0E`.
- Meaning of the final trailer `LE u32 = 1` at payload offset `0x764`.
- Whether item ID `86` / `LAST` is intentionally omitted, represented indirectly, or acts purely as
  a sentinel in the wider item system.
- Exact engine semantics of `.TSG` `SVIT` despite the now-confirmed shared layout.

## Implementation Notes
- Current workspace parser now reads `current_count[n]` from the confirmed record-stride layout for
  selected user-facing items in `SAVEGAME.SAV`, including:
  - Gold
  - Potion of Ironskin
  - Health Potion
  - Strength Potion
  - Aloe
  - Torch
  - Canah Feather
- These are all ordinary `current_count[n]` values inside the wider item table.

## References
- `../SAV_TSG_FULL.md`
- `../../INI/ITEM_INI.md`
