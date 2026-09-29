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

add_executable(battlespire_hash_tests
  battlespire_hash_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/games/battlespire/battlespirevariablehash.cpp
)

target_include_directories(battlespire_hash_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/games/battlespire
)

target_link_libraries(battlespire_hash_tests PRIVATE Qt6::Core)

add_test(NAME battlespire_hash_tests COMMAND battlespire_hash_tests)

add_executable(battlespire_savejoins_tests
  battlespire_savejoins_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/games/battlespire/battlespiresavejoins.cpp
)

target_include_directories(battlespire_savejoins_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/games/battlespire
)

target_link_libraries(battlespire_savejoins_tests PRIVATE Qt6::Core)

add_test(NAME battlespire_savejoins_tests COMMAND battlespire_savejoins_tests)

add_executable(battlespire_bs6_tests
  battlespire_bs6_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/games/battlespire/battlespirebs6.cpp
)

target_include_directories(battlespire_bs6_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/games/battlespire
)

target_link_libraries(battlespire_bs6_tests PRIVATE Qt6::Core)

add_test(NAME battlespire_bs6_tests COMMAND battlespire_bs6_tests)

add_executable(battlespire_3d_tests
  battlespire_3d_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/games/battlespire/battlespire3dbsa.cpp
)

target_include_directories(battlespire_3d_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/xngine
  ${CMAKE_SOURCE_DIR}/src/games/battlespire
)

target_link_libraries(battlespire_3d_tests PRIVATE Qt6::Core xngine)

add_test(NAME battlespire_3d_tests COMMAND battlespire_3d_tests)

add_executable(battlespire_bsi_tests
  battlespire_bsi_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/games/battlespire/battlespirebsi.cpp
)

target_include_directories(battlespire_bsi_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/games/battlespire
)

target_link_libraries(battlespire_bsi_tests PRIVATE Qt6::Core)

add_test(NAME battlespire_bsi_tests COMMAND battlespire_bsi_tests)
