project(modorganizer_game_daggerfall)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Include MO2 base directories
include_directories(${MO2_UIBASE_PATH})
include_directories(${MO2_SRC_PATH})

# Include xngine
include_directories(${CMAKE_SOURCE_DIR}/src/xngine)

# Keep/prune controls:
# - CORE: runtime plugin functionality required for game detection, saves, and BSA-backed mapping.
# - TOOLKIT: parsers/reference tooling useful for advanced validation/authoring workflows.
# - EXE_PATCHING: unsafe/legacy executable patch helpers (explicit opt-in only).
option(XNGINE_DAGGERFALL_ENABLE_TOOLKIT "Enable Daggerfall advanced toolkit parsers" ON)
option(XNGINE_DAGGERFALL_ENABLE_EXE_PATCHING "Enable Daggerfall FALL.EXE patch helpers" OFF)
option(XNGINE_DAGGERFALL_TOOLKIT_TEXT "Enable text/resource toolkit modules" ON)
option(XNGINE_DAGGERFALL_TOOLKIT_QUEST "Enable quest/QBN/QRC toolkit modules" ON)
option(XNGINE_DAGGERFALL_TOOLKIT_WORLD "Enable world-layer toolkit modules" ON)
option(XNGINE_DAGGERFALL_TOOLKIT_AUTHORING "Enable bio/spell/magic authoring toolkit modules" ON)
option(XNGINE_DAGGERFALL_TOOLKIT_ASSETS "Enable image/3D asset toolkit modules" ON)

set(DAGGERFALL_CORE_SOURCES
    gamedaggerfall.cpp
    gamedaggerfall.h
    daggerfallsavegame.cpp
    daggerfallsavegame.h
    daggerfallmapsbsa.cpp
    daggerfallmapsbsa.h
    daggerfallblocksbsa.cpp
    daggerfallblocksbsa.h
    daggerfallcommon.cpp
    daggerfallcommon.h
    daggerfallformatutils.cpp
    daggerfallformatutils.h
)

set(DAGGERFALL_TOOLKIT_TEXT_SOURCES
    daggerfalltextrsc.cpp
    daggerfalltextrsc.h
    daggerfalltextrecord.cpp
    daggerfalltextrecord.h
    daggerfalltextvariables.cpp
    daggerfalltextvariables.h
    daggerfalltextrscindices.cpp
    daggerfalltextrscindices.h
    daggerfallbooktxt.cpp
    daggerfallbooktxt.h
)

set(DAGGERFALL_TOOLKIT_QUEST_SOURCES
    daggerfallqbnpseudo.cpp
    daggerfallqbnpseudo.h
    daggerfallqbn.cpp
    daggerfallqbn.h
)

set(DAGGERFALL_TOOLKIT_WORLD_SOURCES
    daggerfallpak.cpp
    daggerfallpak.h
    daggerfallclimatepak.cpp
    daggerfallclimatepak.h
    daggerfallpoliticpak.cpp
    daggerfallpoliticpak.h
    daggerfallwoodswld.cpp
    daggerfallwoodswld.h
    daggerfallworldlayers.cpp
    daggerfallworldlayers.h
)

set(DAGGERFALL_TOOLKIT_AUTHORING_SOURCES
    daggerfallmagicdef.cpp
    daggerfallmagicdef.h
    daggerfallflatscfg.cpp
    daggerfallflatscfg.h
    daggerfallspellsstd.cpp
    daggerfallspellsstd.h
    daggerfallbiotxt.cpp
    daggerfallbiotxt.h
    daggerfallbiocodes.cpp
    daggerfallbiocodes.h
)

set(DAGGERFALL_TOOLKIT_ASSET_SOURCES
    daggerfallimageformats.cpp
    daggerfallimageformats.h
    daggerfallarch3dbsa.cpp
    daggerfallarch3dbsa.h
)

set(DAGGERFALL_EXE_PATCHING_SOURCES
    daggerfallfallexehacks.cpp
    daggerfallfallexehacks.h
    daggerfallfallexeitems.cpp
    daggerfallfallexeitems.h
)

set(DAGGERFALL_GAME_SOURCES ${DAGGERFALL_CORE_SOURCES})
if(XNGINE_DAGGERFALL_ENABLE_TOOLKIT)
  if(XNGINE_DAGGERFALL_TOOLKIT_TEXT)
    list(APPEND DAGGERFALL_GAME_SOURCES ${DAGGERFALL_TOOLKIT_TEXT_SOURCES})
  endif()
  if(XNGINE_DAGGERFALL_TOOLKIT_QUEST)
    list(APPEND DAGGERFALL_GAME_SOURCES ${DAGGERFALL_TOOLKIT_QUEST_SOURCES})
  endif()
  if(XNGINE_DAGGERFALL_TOOLKIT_WORLD)
    list(APPEND DAGGERFALL_GAME_SOURCES ${DAGGERFALL_TOOLKIT_WORLD_SOURCES})
  endif()
  if(XNGINE_DAGGERFALL_TOOLKIT_AUTHORING)
    list(APPEND DAGGERFALL_GAME_SOURCES ${DAGGERFALL_TOOLKIT_AUTHORING_SOURCES})
  endif()
  if(XNGINE_DAGGERFALL_TOOLKIT_ASSETS)
    list(APPEND DAGGERFALL_GAME_SOURCES ${DAGGERFALL_TOOLKIT_ASSET_SOURCES})
  endif()
endif()
if(XNGINE_DAGGERFALL_ENABLE_EXE_PATCHING)
  list(APPEND DAGGERFALL_GAME_SOURCES ${DAGGERFALL_EXE_PATCHING_SOURCES})
endif()

# Create shared library
add_library(game_daggerfall SHARED
    ${DAGGERFALL_GAME_SOURCES}
    gamedaggerfall.json
)

if(XNGINE_DAGGERFALL_ENABLE_EXE_PATCHING)
  target_compile_definitions(game_daggerfall PRIVATE XNGINE_DAGGERFALL_EXE_PATCHING=1)
endif()

# Link against Qt6
target_link_libraries(game_daggerfall Qt6::Core Qt6::Gui Qt6::Widgets)

# Link against xngine and MO2
target_link_libraries(game_daggerfall
    xngine
    "${MO2_UIBASE_LIB}"
)

# Set output directory
set_target_properties(game_daggerfall PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/plugins"
)

# Enable Qt MOC, UIC, RCC
set_target_properties(game_daggerfall PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)
