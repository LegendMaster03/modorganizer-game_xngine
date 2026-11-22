# Source Library References

Last updated: 2026-03-11

## Referenced Projects
- RGM/patch ecosystem reference:
  - https://github.com/Dillonn241/redguard-mod-manager
- TEXBSI reference project:
  - Redguard-3dfiletest (referenced project; repository URL not recorded)

## External Article Excerpts
- UESP-style `RGM` article excerpt reviewed (2026-03-11), including:
  - full section-order table,
  - section-header framing notes,
  - RAHD/RAAT/RAGR/MPOB/MPRP/MPSL/MPSF/MPMK/MPSZ/WDNM detail tables,
  - enum listings and behavior notes,
  - source-library pointer to `redguard-mod-manager`.
- Merge artifact created:
  - `src/games/redguard/formats/RGM/RGM_UESP_MERGE_NOTES_2026-03-11.md`
- UESP-style `TEXBSI` article excerpt reviewed (2026-03-11), including:
  - image/subrecord framing model,
  - `BSIF`/`IFHD` constraints,
  - `BHDR` value-range table,
  - `DATA` animated offset-list behavior,
  - source-library pointer to `uesp/redguard-3dfiletest`.
- Merge artifact created:
  - `src/games/redguard/formats/TEXBSI_UESP_MERGE_NOTES_2026-03-11.md`
- UESP-style `ROB` article excerpt reviewed (2026-03-11), including:
  - archive overview and map/inventory/menu/test corpus role,
  - `OARC`/`OARD` header table,
  - segment table with embedded-vs-external mode split,
  - explicit `Unknown2` behavior (`0` embedded `3D`, `512` external `3DC`).
- Merge artifact created:
  - `src/games/redguard/formats/ROB_UESP_MERGE_NOTES_2026-03-11.md`
- UESP-style `WLD` article excerpt reviewed (2026-03-11), including:
  - fixed-size whole-file profile (header/section/footer),
  - section/map assembly and per-map role notes,
  - preliminary header/footer field observations,
  - texture-index mapping caveat to TEXBSI numeric image suffixes,
  - source-library pointer to `uesp/redguard-3dfiletest`.
- Merge artifact created:
  - `src/games/redguard/formats/WLD_UESP_MERGE_NOTES_2026-03-11.md`
- UESP-style `3D/3DC` article excerpt reviewed (2026-03-11), including:
  - baseline whole-file section layout and 64-byte header field table,
  - version-split caveats (`<= v2.7` reinterpretations),
  - FaceData/TextureData decoding notes and solid-color modes,
  - `3D` vs `3DC` frame-data behavior notes,
  - source-library pointer to `uesp/redguard-3dfiletest`.
- Merge artifact created:
  - `src/games/redguard/formats/THREED_3DC_UESP_MERGE_NOTES_2026-03-11.md`
- UESP-style `COL` article excerpt reviewed (2026-03-11), including:
  - compact `dword,dword,palette[768]` layout summary,
  - RGB triplet ordering and byte-range note (`0..255`).
- Merge artifact created:
  - `src/games/redguard/formats/COL_UESP_MERGE_NOTES_2026-03-11.md`
- UESP-style `SYSTEM.INI` options article excerpt reviewed (2026-03-11), including:
  - broad section inventory (`[screen]`..`[3dfx]`),
  - option/domain descriptions and defaults (behavior-oriented),
  - preliminary game-unit convention notes (ticks/pixels/custom angles).
- Merge artifact created:
  - `src/games/redguard/formats/INI_UESP_MERGE_NOTES_2026-03-11.md`
- INI folderization pass (2026-03-11):
  - canonical overview moved to `src/games/redguard/formats/INI/INI.md`
  - per-file pages added for `SYSTEM`, `WORLD`, `ITEM`, `MENU`, `COMBAT`, `KEYS`, `REGISTRY`, `surface`, `FOG`, `DIG`, `MDI`
  - compatibility redirect kept at `src/games/redguard/formats/INI.md`
  - retail install scan integrated (root + `fxart/` + `sound/` INIs, with `backup/` mirrors noted)

## Source Files Reviewed

### modorganizer-game_xngine (workspace)
- `src/games/redguard/redguardsavegame.h`
- `src/games/redguard/redguardsavegame.cpp`
- `src/games/redguard/gameredguard.cpp`
- `src/games/redguard/redguardsrtxdatabase.h`
- `src/games/redguard/redguardsrtxdatabase.cpp`
- `src/games/redguard/redguardsmapchanges.h`
- `src/games/redguard/redguardsmapchanges.cpp`
- `src/games/redguard/redguardsmapfile.cpp`
- `src/xngine/xnginesavegame.h`
- `src/xngine/xnginesavegame.cpp`

### redguard-mod-manager-main
- `src/redguard/MapFile.java`
- `src/redguard/MapHeader.java`
- `src/redguard/ParsedMapHeader.java`
- `src/redguard/ScriptReader.java`
- `src/redguard/ScriptParser.java`
- `src/redguard/RtxDatabase.java`
- `src/redguard/RtxEntry.java`
- `src/redguard/MapDatabase.java`
- `src/redguard/Utils.java`
- `src/redguard/TextureFile.java`
- `src/redguard/Texture.java`
- `src/redguard/SoupFlag.java`
- `src/redguard/SoupFunction.java`
- `src/redguard/Item.java`
- `src/redguard/INIFile.java`
- `src/redguard/INIChanges.java`
- `src/redguard/MapChanges.java`
- `src/modManager/RedguardModManager.java`
- `src/modManager/ScriptEditor.java`
- `src/modManager/DialogueEditor.java`

### redguard-3dfiletest-master
- `3DFileTest/Common/RedguardTexBsiFile.h`
- `3DFileTest/Common/RedguardTexBsiFile.cpp`
- `3DFileTest/Common/Redguard3dFile.h`
- `3DFileTest/Common/Redguard3dFile.cpp`
- `3DFileTest/Common/RedguardCommon.h`
- `3DFileTest/Common/RedguardCommon.cpp`
- `3DFileTest/Common/RedguardFbx.h`
- `3DFileTest/Common/RedguardFbx.cpp`
- `3DFileTest/3DFileTest.cpp`

### Redguard (GOG retail install)
- `Redguard/system/*.GXA` (retail corpus count and sample header validation)
- `Redguard/fxart/TEXBSI.*` (sparse numeric shard pattern + header validation)
- `Redguard/maps/*.{RGM,PVO,WLD}` (installed map corpus structure validation)
- `Redguard/SAVEGAME/SAVEGAME.001/SAVEGAME.SAV` (tag-offset validation snapshot: `SVGM`, `THMB`, `SCR3`, `SVCM`, `SVMV`, `SVIT`, `SVMD`)
- `Redguard/REDGUARD.SWG` + `Redguard/redguard.swx` (support-profile blob signatures)
- `Redguard/soup386/SOUP386.DEF` (text-based SOUP definition artifact)
- `Redguard/sound/*` (Miles driver stack signatures: `AIL3DIG`, `AIL3MDI`, config/support files)

### ModOrganizer profile save corpus
- `%LOCALAPPDATA%/ModOrganizer/Redguard (GOG)/profiles/Default/xngine/saves/redguard`
  - scanned nested `SAVEGAME/.001..038` slot directories
  - slot directories present: `38`; `SAVEGAME.SAV` files present: `35`
  - `TSG-only` directories (no `SAVEGAME.SAV`) in current snapshot: `.001`, `.009`, `.025`
  - adjacent-slot comparisons support likely "incomplete/superseded then fixed" pattern (`.001->.002`, `.009->.010`, `.025->.026`)
  - full `TSG` file inventory log generated:
    - `src/games/redguard/formats/TSG_FILES_INDEX.csv`
    - `src/games/redguard/formats/TSG_FILES_INDEX.md`
  - validated known-tag presence/count/offset/length-bounds for:
    - `SVGM`, `THMB`, `SVIT`, `SVMD`, `SCR3`, `SVCM`, `SVMV`, `SVGF`, `SMGR`, `SMOB`, `SMRP`, `SMCO`, `SMSH`, `SMFM`, `SVRG`, `SVGV`
  - observed sidecar files (including `ISLAND.TSG` in some slots) and variable save-file sizes
  - observed corpus-wide thumbnail anomaly: `THMB` present at canonical offset with uniform field bytes `00 01 80 00`, thumbnails corrupted in UI but saves remain loadable

### redguard-unity-master
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/MemoryReader.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/Enum_ModelFileType.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/Q4_28.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RG3DStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGINIStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGMeshStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGRGMStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGSoundStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGTexStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGRTXFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGSFXFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGTEXBSIFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGBSIFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGCOLFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGGXAFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGFNTFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGINIFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGRGMFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGROBFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RG3DFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/RGWLDFile.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGGFXImport/GraphicsConverter.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGObjectStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGRGMAnimStore.cs`
- `redguard-unity-master/Assets/Scripts/RGMData/RGRGMScriptStore.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/RGScriptedObject.cs`
- `redguard-unity-master/Assets/Scripts/RGFileImport/RGMData/soupdeffcn_nimpl.cs`
- `redguard-unity-master/io_tst/sndtst/rtx2int.cs`
- `redguard-unity-master/io_tst/sndtst/rtxtst.cs`
- `redguard-unity-master/io_tst/sndtst/sfxtst.cs`
- `redguard-unity-master/io_tst/3dctst/RG3DFile.cs`
- `redguard-unity-master/io_tst/3dctst/dumpinfo.cs`
- `redguard-unity-master/io_tst/rgmtst/rgmtst.cs`
- `redguard-unity-master/io_tst/rgmtst/scriptstst.cs`
- `redguard-unity-master/io_tst/textst/dumpinfo.cs`
- `redguard-unity-master/io_tst/textst/fnttst.cs`
- `redguard-unity-master/io_tst/textst/RGGXAFile.cs`
- `redguard-unity-master/io_tst/textst/RGFNTFile.cs`
- `redguard-unity-master/io_tst/SOUPDEF_TASKS.cs`
- `redguard-unity-master/io_tst/initst/RGINIFile.cs`
- `redguard-unity-master/io_tst/initst/initst.cs`
- `redguard-unity-master/Assets/Scripts/Save System/SaveLoadManager.cs`
- `redguard-unity-master/Assets/Scripts/Save System/GameSaver.cs`
- `redguard-unity-master/Assets/Scripts/Save System/GameLoader.cs`
- `redguard-unity-master/Assets/Scripts/Save System/SaveGameFree/Scripts/SaveGame.cs`

## Source-Derived Additions
- `RGM.md`: RAHD field offsets used by script/string/variable extraction, writer ordering assumptions, and Java `MAP_CHANGES` sidecar format/apply semantics.
- `TEXBSI.md` + `TEXBSI_FIELDS_FULL.md`: BHDR member naming and CMAP/default palette fallback behavior, plus `redguard-3dfiletest` loader-class and parser-caveat notes (`CRedguardTexBsiFile`/`CRedguardTextureImage` handling for `BHDR`/`CMAP` size usage and load-loop error propagation), and Java `TextureFile` parser/writer heuristics (CMAP probe/skip flow, animated frame-offset byte accounting, nearest-color write fallback).
- `ROB.md`: embedded/external model payload interpretation and 3D/3DC payload structure notes.
  - extended with Unity `RG3DStore` segment handling specifics (embedded `LoadMemory` for `Size>0`, external `<SegmentID>.3DC` fallback for zero-size segments, non-fatal per-segment failure logging).
- `THREED_3DC.md`: Redguard-specific 3D/3DC implementation deltas only (shared/core structure intentionally omitted), including explicit C++ header-slot model (`rg_3dfile_header_t`) and transform constants (`256/256/4096`) used by loader/export path.
  - Added `redguard-3dfiletest` parser specifics: version-split face-field widths, legacy `VertexIndex/12` remap, UV-count derivation from section span, and header/version validation caveats.
- `RTX.md`: full record framing and audio metadata layout from read/write logic, plus `RTX Changes.txt` sidecar shape (`index<TAB>label<TAB>subtitle`) and load-order merge/application caveats from mod-manager apply path.
- `COL.md`: palette container format used by TEXBSI fallback palette loading.
  - added Unity importer filename handling caveat (`SUNSET` palette resolves to lowercase `sunset.col` special-case).
- `GXA.md`: parser-derived BPAL/BBMP layout and image record interpretation.
- `FNT.md`: parser-derived FBMP/RDAT load rules and palette behavior details.
- `PVO.md`: map-scoped PVO corpus presence (`CATACOMB/CAVERNS/DRINT/ISLAND/PALACE`) and recovered known tag set (`OCTH/OCTR/PLST/MLST/END`).
- `LBM.md`: retail `GUI.LBM` presence and conservative `FORM` IFF-container framing guidance.
- `INI.md`: IBM437 parsing/writing and merge semantics used for Redguard INI files in tooling.
- `RGM_SCRIPT_BYTECODE.md`: opcode/operand/comparison/operator/object selector mapping for RASC, plus concrete `currentTask+paramIndex` typing keys and label backfill-width caveat from Java parser/writer behavior.
    - Extended with Unity `io_tst/rgmtst` opcode-switch coverage notes: explicit handled opcode subset in `RGRGMScriptStore.readInstruction`, known gaps (`0x0F/0x10`, object-selector `0x0A`), and placeholder-semantics opcodes (`ENDINT`/`TASKPAUSE`/`SCRIPTRV`).
  - Extended with Unity VM loop/control-flow notes from `ScriptData`: `runScript` termination sentinel flow (`0xDEAD`), `readEnd` persistent `scriptPC` update, gosub/return callstack behavior, and instruction-count bailout guard.
  - Added non-SAV open-questions section and targeted validation plan for unresolved opcode/selector paths (`0x0F/0x10`, selector `0x0A`, reference-byte truncation, ignored LHS/RHS operator bytes).
- `SAV_TSG.md`: expanded with parser-accurate Redguard save layout/metadata extraction flow, chunk heuristics, RTX-backed location resolution, and explicit full-vs-partial parser status.
  - Added explicit parser coverage boundary from source scan: current runtime metadata parser hardcodes `THMB`, `SVIT`, `SVMD` only; other known tags remain opaque/unhandled.
  - Added implemented chunk-table dumper utility (`RedguardsSaveGame::chunkTableDebugText`) reporting tag/offset/length/bounds/decode-status rows for current `SAVEGAME.SAV`.
  - Added retail GOG save validation snapshot with concrete marker offsets from `SAVEGAME.001/SAVEGAME.SAV`.
  - Added extended MO2 save-corpus validation: known-tag one-occurrence pattern across current MO2 save files, bounded BE-length sanity checks, variable-length block evidence (`SMGR/SMOB/SMRP/SVRG`), non-fatal THMB corruption pattern notes, and observed `TSG-only` directories now treated as likely incomplete/superseded snapshots.
  - Normalized structure to template-aligned sections (`Container/File Layout`, `Section/Block Order`, `Common Header`, `Cross-Block Relationships`, `Discrepancies`, `Implementation Status`, `Open Questions`, `References`) while preserving existing technical evidence.

- `FORMAT_DOC_TEMPLATE.md`
  - Added a reusable canonical format-document skeleton for cleanup passes across remaining Redguard format docs.
- `TSG_FILES_INDEX.csv` + `TSG_FILES_INDEX.md`: complete logged inventory of discovered `*.TSG` files (path/slot/size/timestamp) for the current save corpus.
- `SAVE_PAIR_DIFFS.md`: explicit non-`SAVEGAME.SAV` diffs for suspect broken->fixed slot pairs (`.001->.002`, `.009->.010`, `.025->.026`).
- `SWG_SWX.md`: retail `SWG/SWX` binary signature observations and current interpretation status.
- `SOUP386_DEF.md`: retail `SOUP386.DEF` text/signature observations and relation to SOUP function catalogs, including Java script-bytecode coupling for SOUP-derived flag indices/opcodes (`6`/`22`) and flag-parameter padding behavior.
- `SOUND_MILES_STACK.md`: retail sound-driver stack inventory/signatures (`.dig/.mdi/.ad/.opl/.ini/.lst/.rst`) distinct from `MAIN.SFX` container docs.
- `SFX_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `MAIN.SFX` ingestion.
- `FNT_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `FNT` ingestion.
- `ROB_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `ROB` ingestion.
- `WLD_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `WLD` ingestion.
- `THREED_3DC_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `3D/3DC` ingestion.
- `COL_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `COL` ingestion.
- `INI_UESP_MERGE_NOTES_2026-03-11.md`: provenance/delta log for UESP `SYSTEM.INI` ingestion.
- `INI/README.md`: INI folder index + retail install INI inventory snapshot.
- `RETAIL_FILESET_COVERAGE.md`: consolidated “documented vs pending” mapping for all major observed retail file families.
- Unity cross-check deltas:
  - `RTX.md`: `END ` sentinel compare style and `HasAudio == 256` interpretation in Unity parser.
  - `SFX.md`: enum mappings, strict `FXHD.numSounds`-driven record iteration, and io_tst 8-bit PCM normalization reference.
  - `TEXBSI.md`: nested image-size-driven BSI parsing and animated DATA row-offset seek behavior.
  - `GXA.md`: `BMHD` field interpretation, `BMHD.numImages`-driven BBMP item count, and ranked unknown-field RE priorities from io_tst corpus dumps.
    - Added retail install validation (`system/` GXA corpus count and `GUI.GXA` header bytes).
  - `FNT.md`: strict chunk iteration, `FNHD.numCharacters`-driven FBMP record count, io_tst corpus-validation coverage, and ranked FNHD/RDAT RE priorities.
  - `THREED_3DC.md`: Redguard-focused deltas including textureData decode behavior, legacy compatibility paths, and unresolved unknown-data/frame metadata notes from Unity `io_tst`.
    - added Unity `RG3DStore` mesh-store transform conventions (`1/5120` scale factor and axis/rotation flip vectors) and store-level `.3DC` load-bias assumptions.
  - `RGM_SCRIPT_BYTECODE.md`: independent VM/opcode model confirmation from Unity script store.
  - `INI.md`: typed key schemas for `ITEM.INI`, `WORLD.INI`, and `MENU.INI` from Unity INI parser/store.
    - includes io_tst additions: `world_sky_xrotate/world_sky_yrotate`, semicolon comment stripping behavior, and `[general]`/`[page0]` handler coupling details.
  - `RGM.md`: additional section/record semantics, runtime coordinate/rotation conversions, io_tst pointer arithmetic checks for `RANM`/`RALC`, and documented `MAP_CHANGES` sidecar parity/caveats across Java + workspace C++ implementations.
    - Extended with Unity `RGRGMFile` parser mechanics: length-derived item-count rules (`MPSZ`/`RALC`/`RAEX`/`RAVC`), `MPMK` trailing flag-byte read, `RAHD` trailing unknown `u32`, non-fatal unknown-section skip-by-seek behavior, and `WDNM` dual-node-count read with `nodeCount1`-driven iteration.
    - Extended with Unity `RGRGMAnimStore` decoding notes: `RAAN` model-file token normalization, `RAGR` sentinel termination (`PeekInt16()==0`), and explicit 3-byte frame bitfield unpacking (`type/value/timescale/modifier`).
    - Normalized doc layout to template-aligned structure (`Container / File Layout`, `Section / Block Order`, `Common Header`, `Cross-Block Relationships`, `Discrepancies`, `Corpus Validation Notes`, `Implementation Status`, `Open Questions`, `References`) without dropping existing extracted evidence.
    - Moved section-specific Unity/io_tst notes out of bottom appendix and into their corresponding block sections (`RAHD`, `RALC`, `RAEX`, `RAAT`, `RAAN`, `RAGR`, `RANM`, `RAVC`, `MPOB`, `MPSO`, `MPSF`, `MPMK`, `MPSZ`, `WDNM`) to keep per-section behavior localized.
    - Integrated `redguard-mod-manager` writer/parser notes from appendix into relevant block sections (`RAHD`, `RAFS`, `RAST`, `RASB`, `RASC`, `RAHK`, `RALC`, `RAEX`, `RAAN`, `RAGR`, `RANM`, `RAVC`, `MPOB`, `MPRP`, `MPSO`, `MPSL`, `MPSF`, `MPMK`, `MPSZ`, `WDNM`, `FLAT`) and removed redundant standalone implementation appendix.
    - Refined `RAHD` section into structured spec form with section-layout table, record-key-field table, explicit pointer-hub semantics, and dedicated unknowns list.
    - Expanded additional under-modeled blocks with external-doc field detail in-section (no appendix spill): `RAHK` mixed-form layout + unknowns, `RAEX` key-field table, `RAAT` behavioral highlights, `RAGR` group/frame bitfield layouts + enum highlights, `WDNM` walk-map/walk-node tables, plus stronger `MPRP`/`MPSL`/`MPSZ` record highlights and `RAVC` signed-offset type correction (`s8`).
    - Added dedicated `RGM/` doc set for block-level decomposition: new `RGM/README.md` plus per-block files under `RGM/blocks/` (`RAHD` through `END`), while keeping top-level `RGM.md` as the general overview and index.
    - Relocated main RGM overview to `RGM/RGM.md`, left `RGM.md` as compatibility redirect stub, and updated format index links.
    - Backfilled block docs using remaining companion sources (`RGM_BEHAVIOR_NOTES.md`, `RGM_ENUMS_NOTES.md`): added concrete behavior/enum semantics for `RAHK`, `RALC`, `RAEX`, `RAAN`, `RAGR`, `RANM`, `RAVC`, `MPOB`, `MPRP`, `MPSL`, `MPMK`, and `WDNM`.
    - Began moving companion-file payload into block-local docs: embedded full RAAT attribute index map directly in `RGM/blocks/RAAT.md`, added opcode/runtime summaries in `RGM/blocks/RASC.md`, and expanded block-local field snapshots (`MPOB`, `MPSO`, `MPSF`, `RAVA`, `RANM`) so key details are no longer only in aggregate companion files.
    - Completed RAAT consolidation: merged full RAAT attribute index and per-offset behavior/range table into `RGM/blocks/RAAT.md`, removed standalone `RGM_RAAT_ATTRIBUTES_FULL.md` and `RGM_RAAT_BEHAVIOR_NOTES.md`, and updated all references to the block-local canonical location.
    - Migrated additional `RGM_FIELDS_FULL` section payloads into canonical block docs (`RAHD`, `MPRP`, `MPSL`, `MPMK`, `MPSZ`, `WDNM`) and repurposed `RGM_FIELDS_FULL.md` as a migration index that routes to `blocks/<TAG>.md` files.
    - Applied another field-migration pass using extended external notes: expanded block-local semantics for `RAST`, `RASB`, `RAVA`, `RAHK`, `RAAN`, `RAGR`, `RAVC`, `MPOB`, `MPRP`, `MPSL`, `MPSF`, `MPSZ`, and `WDNM`; restored `RGM_FIELDS_FULL.md` as compatibility index when missing.
    - Completed behavior-notes flattening: moved remaining unique behavior nuggets into block docs (`RAHK`, `RAEX`, `MPSO`) and converted `RGM_BEHAVIOR_NOTES.md` to a compatibility migration index pointing to `blocks/*.md` and `RGM.md`.
  - `RGM_TASK_FUNCTION_RUNTIME.md`: script task/function dispatch model, implemented handler ID map, and fallback SOUP function index map.
    - includes validator-priority tiers (A/B/C) and recommended install-validation order.
    - extended with `io_tst/SOUPDEF_TASKS.cs` domain coverage (ID naming/arity/task-vs-function classification support).
    - includes concrete parameter-annotation captures (`lockoutPlayer`, `RotateByAxis`, `MoveByAxis`, `Wait`, `ACTIVATE`, animation wait/push tasks, `SyncWithGroup`) with explicit note that this source labels itself speculative.
    - added Java toolchain cross-check (`MapDatabase`/`ScriptReader`/`ScriptParser`) for synthetic index-0 `NullFunction` SOUP ingestion and opcode branching rules (`task=0`, `function=2`, `@multitask=1`, object-dot `25/26`).
    - added Java flag/global-flag opcode notes: flag references (`6 + u16 index`), parameter-mode flag padding short, and global-flag numeric opcode specialization (`22` for `TestGlobalFlag`/`SetGlobalFlag`/`ResetGlobalFlag`).
    - added Unity `io_tst/rgmtst` reader-limit notes (hardcoded `object_me` subject for top-level task opcodes and unresolved placeholder handling for operator-byte semantics, `ENDINT`, `TASKPAUSE`, `SCRIPTRV`).
    - extended with concrete `rgmtst` opcode coverage table from `readInstruction` and harness-scope caveat for `scriptstst.cs` full `367`-slot SOUPDEF dispatch table (routing/name coverage vs behavior parity).
    - extended with Unity `ScriptData` execution-loop semantics: end/goto IP behavior, gosub/return stack mechanics, single-tick mode, and loop-guard bailout behavior.
  - `WLD.md`: new parser-derived world terrain format spec from Unity `RGWLDFile` + mesh build path.
    - added importer nonlinear height function coefficients and explicit mesh-build assumptions (`64` texture buckets + hardcoded UV rotation tables).
  - `TEXBSI.md`: ranked BHDR/IFHD-BSIF/animation-offset RE priorities and explicit note that no dedicated io_tst BSI harness is present.
    - Added retail install validation for sparse `TEXBSI.###` shard corpus (count/range/gaps + header sample).
    - Added Unity `RGBSIFile` behavior notes: unknown BSI segment names are fatal; DATA random-access reads realign stream position after payload; CMAP path expects full 768-byte palette in that flow.
  - `SFX.md`: added Unity `RGSFXFile` behavior notes that unknown top-level sections are skipped by bounded seek and PCM payload is ingested as raw bytes independent of declared bit depth.
  - `RTX.md`: added Unity `RGRTXFile` behavior notes that entries are indexed by packed integer labels and parsing proceeds field-by-field after reading `TotalLength`.
  - `INI.md`: added Unity `RGINIFile` line-tokenization caveat (`Split('=')` first-two-token handling) and unknown indexed-key logging/ignore behavior.
  - `REDGUARD_SHARED_NOTES.md`: added Unity `MemoryReader` primitive model notes (little-endian scalar default, explicit per-field byte reversal for big-endian lengths, and bounds-checked seek behavior).

  - `RESTORATION_EVIDENCE_MATRIX.md`: project-backed matrix distinguishing parsed/used fields vs preserved-opaque fields for the first extraction wave (`redguard-mod-manager-main` + `redguard-3dfiletest-master`), including prioritized next-step queue.

## Consolidated External-Source Merge Notes

The removed date-stamped merge logs were reference artifacts rather than canonical specifications. Their remaining useful provenance and caveats are retained here:

- RGM: UESP-style notes confirmed section order and many field narratives, but RAHD/RAAT/RAST etc. remain external guidance unless byte-level corpus or implementation parity is documented in the block pages. The ROB merge log's 36-byte-header claim is superseded by the canonical ROB 20-byte header plus 80-byte segment headers validated in retail corpus.
- TEXBSI: The UESP-style model confirms image/subrecord framing and `BHDR`/`DATA` structure; claims about specific `TEXBSI.412` semantic ownership remain unconfirmed and are not promoted without asset/runtime mapping.
- ROB: External guidance confirmed the 20-byte header, segment framing, and embedded/external model split. Unknown header/segment fields remain opaque.
- WLD: External fixed-size observations are corroborating evidence, but implementation-grounded layout and runtime behavior remain authoritative.
- 3D/3DC: Version-split and face/texture decoding details are retained in `THREED_3DC.md`; uncertain labels remain explicitly caveated.
- COL: External layout aligns with the two-dword plus 768-byte palette model, while parser-tolerant handling is documented in `COL.md`.
- INI: External option inventory is behavior-oriented and partly speculative; code-derived parsing/writing semantics are canonical.
- FNT/SFX: External merge notes confirmed framing and catalogs; unresolved FNHD/RDAT fields and per-effect byte order remain caveated in canonical pages.
