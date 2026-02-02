add_library(game_arena MODULE)

target_sources(game_arena PRIVATE
  gamearena.h
  gamearena.cpp
  # Game-specific feature classes disabled
  # arenasmoddatachecker.h
  # arenasmoddatacontent.h
  # arenasmoddatacontent.cpp
  # arenasavegame.h
  # arenasavegame.cpp
)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

target_include_directories(game_arena PRIVATE
  ${MO2_UIBASE_PATH}
  ${MO2_SRC_PATH}
  ${CMAKE_SOURCE_DIR}/src/xngine
)

target_link_libraries(game_arena PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  xngine
  "${MO2_UIBASE_LIB}"
)

set_target_properties(game_arena PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Release/plugins
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Release/plugins
)

target_compile_definitions(game_arena PRIVATE
  WIN32
  _WINDOWS
  QT_CORE_LIB
  QT_GUI_LIB
  QT_WIDGETS_LIB
)
