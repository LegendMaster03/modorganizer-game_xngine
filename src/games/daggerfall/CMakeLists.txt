cmake_minimum_required(VERSION 3.16)

project(modorganizer_game_daggerfall)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Include MO2 base directories
include_directories(${MO2_UIBASE_PATH})
include_directories(${MO2_SRC_PATH})

# Include xngine
include_directories(${CMAKE_SOURCE_DIR}/src/xngine)

# Source files for Daggerfall game plugin
set(DAGGERFALL_GAME_SOURCES
    gamedaggerfall.cpp
    gamedaggerfall.h
    # daggerfallsmoddatachecker.h  # Disabled - using XnGine base
    # daggerfallsmoddatacontent.cpp  # Disabled - using XnGine base
    # daggerfallsmoddatacontent.h  # Disabled - using XnGine base
)

# Create shared library
add_library(game_daggerfall SHARED
    ${DAGGERFALL_GAME_SOURCES}
    gamedaggerfall.json
)

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
