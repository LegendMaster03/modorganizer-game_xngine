find_package(Qt6 COMPONENTS Core REQUIRED)

add_executable(xngine_format_tests
  xngine_format_tests.cpp
)

target_include_directories(xngine_format_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/xngine
  ${CMAKE_SOURCE_DIR}/src/games/battlespire
)

target_link_libraries(xngine_format_tests PRIVATE
  Qt6::Core
  xngine
)

add_test(NAME xngine_format_tests COMMAND xngine_format_tests)
