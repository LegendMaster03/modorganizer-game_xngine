add_library(game_battlespire MODULE)

target_sources(game_battlespire PRIVATE
  gamebattlespire.h
  gamebattlespire.cpp
  battlespiresavegame.h
  battlespiresavegame.cpp
  # Game-specific feature classes disabled
  # battlespiredatachecker.h
  # battlespiremodatacontent.h
  # battlespiremodatacontent.cpp
  # battlespireafegame.h
  # battlespireafegame.cpp
)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

target_include_directories(game_battlespire PRIVATE
  ${MO2_UIBASE_PATH}
  ${MO2_SRC_PATH}
  ${CMAKE_SOURCE_DIR}/src/xngine
)

target_link_libraries(game_battlespire PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  xngine
  "${MO2_UIBASE_LIB}"
)

set_target_properties(game_battlespire PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Release/plugins
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Release/plugins
)

target_compile_definitions(game_battlespire PRIVATE
  WIN32
  _WINDOWS
  QT_CORE_LIB
  QT_GUI_LIB
  QT_WIDGETS_LIB
)
