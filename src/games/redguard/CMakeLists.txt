project(modorganizer_game_redguard)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Include MO2 base directories
include_directories(${MO2_UIBASE_PATH})
include_directories(${MO2_SRC_PATH})

# Include xngine
include_directories(${CMAKE_SOURCE_DIR}/src/xngine)

# Source files for Redguard game plugin
set(REDGUARD_GAME_SOURCES
    gameredguard.cpp
    gameredguard.h
    redguardsavegame.cpp
    redguardsavegame.h
    redguardsmapchanges.cpp
    redguardsmapchanges.h
    redguardsmapdatabase.cpp
    redguardsmapdatabase.h
    redguardsmapfile.cpp
    redguardsmapfile.h
    redguardsmapheader.cpp
    redguardsmapheader.h
    redguardsparsedmapheader.h
    redguardsscriptinstruction.cpp
    redguardsscriptinstruction.h
    redguardsscriptparser.cpp
    redguardsscriptparser.h
    redguardsscriptreader.cpp
    redguardsscriptreader.h
    redguardssoupflag.cpp
    redguardssoupflag.h
    redguardssoupfunction.cpp
    redguardssoupfunction.h
    redguardsitem.cpp
    redguardsitem.h
    redguardsrtxdatabase.cpp
    redguardsrtxdatabase.h
    redguardswld.cpp
    redguardswld.h
    redguardsutils.cpp
    redguardsutils.h
    redguardsmoddatachecker.h
    redguardsmoddatacontent.cpp
    redguardsmoddatacontent.h
)

# Create shared library
add_library(game_redguard SHARED
    ${REDGUARD_GAME_SOURCES}
    gameredguard.json
)

# Link against Qt6
target_link_libraries(game_redguard Qt6::Core Qt6::Gui Qt6::Widgets)

# Link against xngine and MO2
target_link_libraries(game_redguard
    xngine
    "${MO2_UIBASE_LIB}"
)

# Set output directory
set_target_properties(game_redguard PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/plugins"
)

# Enable Qt MOC, UIC, RCC
set_target_properties(game_redguard PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)
