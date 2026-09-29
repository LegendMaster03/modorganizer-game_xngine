add_library(game_battlespire MODULE)

target_sources(game_battlespire PRIVATE
  gamebattlespire.h
  gamebattlespire.cpp
  battlespiresaveformat.h
  battlespiresavejoins.h
  battlespiresavejoins.cpp
  battlespiresavegame.h
  battlespiresavegame.cpp
  battlespirevariablehash.h
  battlespirevariablehash.cpp
  battlespirebs6.h
  battlespirebs6.cpp
  battlespirebsi.h
  battlespirebsi.cpp

  # Speculative Daggerfall-derived compatibility adapters. No confirmed Battlespire
  # TEXT.RSC/WATER.TBL resources or production callers are known; verified water data
  # is represented by BS6 WATR chunks. Keep these isolated until fixtures prove use.
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
