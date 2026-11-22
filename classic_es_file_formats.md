---

## Formats Used by Multiple Games but Handled in a Single Plugin

The following formats are currently supported in a specific game plugin but are used by more than one XnGine game. These are strong candidates for elevation to the core:

| Extension | Games Using | Currently Handled In | Recommendation |
|-----------|------------|---------------------|----------------|
| .WLD      | Daggerfall, Redguard | Daggerfall plugin | Elevate to core |
| .RSC      | Daggerfall, Battlespire | Daggerfall plugin | Elevate to core |
| .TBL      | Daggerfall, Battlespire | Daggerfall plugin | Elevate to core |
| .SND      | Daggerfall, Battlespire | Daggerfall plugin | Elevate to core |
| .FNT      | Arena, Battlespire, Terminator:SN | Arena/Battlespire plugin | Elevate to core |
| .INF      | Arena, Daggerfall | Arena/Daggerfall plugin | Elevate to core |
| .CEL      | Arena, Daggerfall | Arena/Daggerfall plugin | Elevate to core |
| .CFA      | Daggerfall, Arena | Daggerfall plugin | Elevate to core |
| .RCI      | Arena, Daggerfall | Arena/Daggerfall plugin | Elevate to core |
| .CIF      | Daggerfall, (possibly others) | Daggerfall plugin | Review for core |

This table is not exhaustive; as more games are analyzed, additional candidates may be found. Elevating these formats to the core will improve cross-game support and code maintainability.

# Classic XnGine Game File Formats

This document lists and organizes the file formats used by classic XnGine games, with notes on their purposes, which games use them, and how the engine evolved over time. Many formats handled by the core XnGine code are already included in the core section below.

---

## Known XnGine Games (with Release Years)

| Game Title                                         | Release Year |
|----------------------------------------------------|--------------|
| The Terminator: Future Shock                       | 1995         |
| The Elder Scrolls II: Daggerfall                   | 1996         |
| XCar: Experimental Racing                          | 1997         |
| An Elder Scrolls Legend: Battlespire               | 1997         |
| Burnout: Championship Drag Racing                  | 1998         |
| The Elder Scrolls Adventures: Redguard             | 1998         |
| Skynet (video game)                                | 1996/1997*   |
| NIRA Intense Import Drag Racing                    | 1999         |
| The 10th Planet (unreleased)                       | N/A          |

*Note: Skynet is sometimes listed as 1996 or 1997 depending on region/source.

The release year is important for understanding how the engine and its file formats evolved. Earlier games may lack features or formats introduced in later titles.

---

## Table of Contents
- [Shared Formats](#shared-formats)
- [Arena Formats](#arena-formats)
- [Daggerfall Formats](#daggerfall-formats)
- [Battlespire Formats](#battlespire-formats)
- [Redguard Formats](#redguard-formats)

---

## Shared Formats (Likely XnGine Core)
| Extension | Games | Purpose |
|-----------|-------|---------|
| .BSA      | Arena, Daggerfall, Battlespire, Redguard, Terminator:FS, Terminator:SN | Archive format for game assets (textures, models, etc.) |
| .3D       | Battlespire, Redguard, Terminator:FS | 3D static model files (items, props, inventory, world objects) |
| .3DC      | Redguard | 3D character/creature model files (animated/articulated models) |
| .FLC      | Arena, Daggerfall, Battlespire, Terminator:FS | Animation/video files (Autodesk Animator format) |
| .IMG      | Daggerfall, Arena, Terminator:FS | Image/texture files |
| .CIF      | Daggerfall | Image container (sprites, etc.) |
| .RCI      | Arena, Daggerfall | Resource container/image |
| .RGM      | Redguard | Region/level model |
| .WLD      | Daggerfall, Redguard | World/level data |
| .PVO      | Redguard | Portal/level data |
| .BS6      | Battlespire | Level/model data |
| .SCL      | Battlespire | Class data |
| .H2L      | Battlespire | Level data |
| .QBN/.QRC | Daggerfall | Quest binary/compiled quest |
| .RSC      | Daggerfall, Battlespire | Resource files |
| .TDE      | Daggerfall | Text data |
| .STD      | Daggerfall | Standard data |
| .DAT      | All | Generic data files (various uses: config, assets, etc.) |
| .BIN      | Daggerfall, Redguard, Terminator:FS | Binary data (various uses) |
| .RAW      | Daggerfall, Battlespire, Terminator:FS | Raw binary data (often textures or palettes) |
| .COL      | Arena, Daggerfall, Battlespire, Terminator:FS | Color palette or collision data |
| .PAL      | Arena, Daggerfall, Battlespire, Terminator:FS | Palette files |
| .LGT      | Arena, Daggerfall, Terminator:FS | Lighting data |
| .TBL      | Daggerfall, Battlespire | Table data (various uses) |
| .GFX      | Daggerfall | Graphics data |
| .VOC      | Arena | Audio (Creative Voice file) |
| .BNK      | Arena, Daggerfall | Sound bank files |
| .SND      | Daggerfall, Battlespire | Sound data |
| .FNT      | Arena, Battlespire, Terminator:SN | Font files |
| .INF      | Arena, Daggerfall | Info/configuration |
| .PAK      | Daggerfall | Packed data |
| .CEL      | Arena, Daggerfall | Animation cell files |
| .CFA      | Daggerfall | Animation frames |
| .EMS      | Arena | Memory manager data |
| .NTZ      | Arena | Data (unknown, possibly music) |
| .XFM      | Arena | Data (unknown) |
| .OPL/.AD  | Arena | Audio data |
| .LST      | Arena | List files |
| .TXT      | All | Text data, scripts, logs |
| .INI      | All | Configuration files |
| .CFG      | All | Configuration files |
| .EXE      | All | Executable files |
| .BAT      | All | Batch scripts |
| .ICO      | All | Icon files |
| .LOG      | All | Log files |
| .MSG      | All | Message files |
| .SCR      | Daggerfall | Script files |
| .CPY      | Arena | Copy/backup files |
| .BAK      | All | Backup files |
| .SAV      | Daggerfall | Save files |
| .SWG/.SWX | Redguard | Game data (unknown) |
| .TSG      | Redguard | Script/game data |
| .INS      | All | Installer data |
| .HMI      | Terminator:SN | MIDI music (Human Machine Interfaces) |
| .WAV      | Terminator:SN | Wave audio |
| .PRC/.PRS | Terminator:FS, Terminator:SN | Compressed/printer or script data |
| .MAP/.210 | Terminator:SN | Map/level data |
| .IM2      | Terminator:SN | Image or map data |

---

## Arena Formats
- .BSA: Asset archive
- .MIF: Map/level data
- .VOC: Audio (voice, sound effects)
- .BNK: Sound bank
- .FLC: Animation (cutscenes, intros)
- .DAT: Data/config
- .LGT: Lighting data
- .RCI: Resource container (images, sprites)
- .COL: Palette/collision
- .OPL/.AD: Audio data
- .FNT: Fonts
- .PAL: Palette
- .IMG: Images
- .TXT: Text/scripts
- .LST: List files
- .INF: Info/configuration
- .EMS: Memory manager data
- .NTZ: Possibly music or data
- .XFM: Unknown data
- .CPY: Copy/backup files
- .BAK: Backup files
- .EXE/.BAT: Executables/scripts
- .ICO: Icon files

## Daggerfall Formats
- .BSA: Asset archive
- .CIF: Image container (sprites, textures)
- .IMG: Images
- .DAT: Data/config
- .QBN/.QRC: Quest data (binary/compiled)
- .WLD: World/level data
- .TBL: Table data
- .PAK: Packed data
- .RSC: Resource files
- .SCR: Script files
- .SND: Sound data
- .TDE: Text data
- .STD: Standard data
- .SAV: Save files
- .COL: Palette/collision
- .PAL: Palette
- .LGT: Lighting
- .RAW: Raw binary data
- .GFX: Graphics data
- .INF: Info/configuration
- .CEL: Animation cell files
- .CFA: Animation frames

## Battlespire Formats
- .BSA: Asset archive
- .3D: 3D static models (items, props, world objects)
- .BS6: Level/model data
- .SCL: Class data
- .H2L: Level data
- .RSC: Resource files
- .DAT: Data/config
- .FLC: Animation
- .FNT: Fonts
- .COL: Palette/collision
- .RAW: Raw data
- .TXT: Text/scripts
- .PAL: Palette
- .BIN: Binary data


## Redguard Formats
- .RGM: Region/level model
- .WLD: World/level data
- .PVO: Portal/level data
- .3D: 3D static model (items, inventory, props)
- .3DC: 3D character/creature model (animated, articulated)
- .SWG/.SWX: Game data (unknown)
- .TSG: Script/game data
- .BIN: Binary data
- .DAT: Data/config
- .INI/.CFG: Config
- .EXE/.BAT: Executables/scripts
- .TXT: Text/scripts
- .ICO: Icon files
- .LOG: Log files

---

## Documentation Scope and Status

The lists in this document are a cross-game format inventory. “Likely” or “unknown” classifications are research labels, not claims that a parser or plugin implementation is complete. Where this repository has a dedicated implementation, use the current game plugin source and the game-specific format documentation as the authority.

## Notes
- Many formats are reused with different purposes between games.
- .BSA is the most widely shared archive format and is present in all XnGine games reviewed.
- .3D, .IMG, .RAW, .COL, .PAL, .LGT, .DAT, .BIN, .FNT, .LOG, .INI, .CFG, .EXE, .BAT, .TXT, .BAK, .CPY, .MSG, .ICO, .SCR, .SAV, .TBL, .VOC, .BNK, .FLC, .RSC, .RCI, .CIF, .GFX, .PAK, .INF, .CEL, .CFA, .EMS, .NTZ, .XFM, .OPL/.AD, .LST, .QBN/.QRC, .SCL, .BS6, .H2L, .WLD, .RGM, .PVO, .SWG/.SWX, .TSG, .TDE, .STD, .HMI, .WAV, .PRC/.PRS, .MAP, .IM2 are all found in at least one XnGine game.
- Some formats (.3DC, .RGM, .PVO, .SWG, .TSG) are unique to Redguard and may be game-specific.
- .QBN/.QRC are Daggerfall's quest formats.
- .BS6, .SCL, .H2L are Battlespire-specific for levels, classes, and models.
- .HMI and .WAV are used for music/audio in Terminator: SkyNET.
- .PRC/.PRS, .MAP, .IM2 are seen in Terminator games and may be for levels, scripts, or compressed data.
- This list is not exhaustive; some files may have specialized or unknown uses or be used for multiple purposes depending on the game.
- Comparisons across XnGine games can help identify which formats are likely part of the engine core and which are game-specific extensions.
