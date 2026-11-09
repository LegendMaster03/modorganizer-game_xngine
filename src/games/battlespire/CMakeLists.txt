add_library(game_battlespire MODULE)

target_sources(game_battlespire PRIVATE
  gamebattlespire.h
  gamebattlespire.cpp
  # Shared SAVEVARS/SAVETREE layout constants and cross-file ID joins.
  battlespiresaveformat.h
  battlespiresavejoins.h
  battlespiresavejoins.cpp
  battlespiresavegame.h
  battlespiresavegame.cpp
  battlespirevariablehash.h
  battlespirevariablehash.cpp
  # Experimental readers for Battlespire level and image resources. Their
  # implemented layouts are hypotheses pending validation with original assets;
  # they are independent of the Daggerfall-derived compatibility adapters below.
  battlespirebs6.h
  battlespirebs6.cpp
  battlespirebsi.h
  battlespirebsi.cpp

  # Daggerfall-derived compatibility hypotheses, not confirmed Battlespire formats.
  # The research branch also experiments with BS6 WATR interpretation, but neither
  # that parser nor these adapters establish original-game behavior without fixtures.
  battlespiretextrsc.h
  battlespiretextrsc.cpp
  battlespirewatertbl.h
  battlespirewatertbl.cpp

  battlespire3dbsa.h
  battlespire3dbsa.cpp
  battlespiredatachecker.h
  battlespiremodatacontent.h
  battlespiremodatacontent.cpp
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
