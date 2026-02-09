cmake_minimum_required(VERSION 3.16)

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
    # redguardsmoddatachecker.h  # Disabled - using XnGine base
    # redguardsmoddatacontent.cpp  # Disabled - using XnGine base
    # redguardsmoddatacontent.h  # Disabled - using XnGine base
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
