project(modorganizer_game_arena)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Include MO2 base directories
include_directories(${MO2_UIBASE_PATH})
include_directories(${MO2_SRC_PATH})

# Include xngine
include_directories(${CMAKE_SOURCE_DIR}/src/xngine)

# Source files for Arena game plugin
set(ARENA_GAME_SOURCES
    gamearena.cpp
    gamearena.h
)

# Create shared library
add_library(game_arena SHARED
    ${ARENA_GAME_SOURCES}
    gamearena.json
)

# Link against Qt6
target_link_libraries(game_arena Qt6::Core Qt6::Gui Qt6::Widgets)

# Link against xngine and MO2
target_link_libraries(game_arena
    xngine
    "${MO2_UIBASE_LIB}"
)

# Set output directory
set_target_properties(game_arena PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release/plugins"
)

# Enable Qt MOC, UIC, RCC
set_target_properties(game_arena PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)