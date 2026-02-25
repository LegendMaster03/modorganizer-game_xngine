cmake_minimum_required(VERSION 3.16)

project(bsa_extract_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core REQUIRED)

add_executable(bsa_extract_cli
  main.cpp
  ../../src/xngine/xnginebsaformat.cpp
  ../../src/xngine/xnginebsaformat.h
)

target_include_directories(bsa_extract_cli PRIVATE
  ../../src/xngine
)

target_link_libraries(bsa_extract_cli PRIVATE
  Qt6::Core
)
