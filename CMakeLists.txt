cmake_minimum_required(VERSION 3.16)

project(game_xngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt SDK path (override via -DQT_ROOT or -DCMAKE_PREFIX_PATH)
set(QT_ROOT "" CACHE PATH "Path to Qt SDK root")
if(QT_ROOT AND EXISTS "${QT_ROOT}")
  list(PREPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# MO2 SDK paths - configure these paths to point to your MO2 development directories
set(MO2_UIBASE_PATH "" CACHE PATH "Path to MO2 uibase source directory")
set(MO2_UIBASE_INCLUDE "" CACHE PATH "Path to MO2 uibase include directory")
set(MO2_SRC_PATH "" CACHE PATH "Path to MO2 source directory")
set(MO2_UIBASE_LIB "" CACHE PATH "Path to MO2 uibase.lib")

if(NOT MO2_UIBASE_PATH)
  message(FATAL_ERROR "Please set MO2_UIBASE_PATH")
endif()
if(NOT MO2_UIBASE_INCLUDE)
  set(MO2_UIBASE_INCLUDE "${MO2_UIBASE_PATH}/include/uibase" CACHE PATH "Path to MO2 uibase include directory" FORCE)
endif()
if(NOT MO2_SRC_PATH)
  message(FATAL_ERROR "Please set MO2_SRC_PATH")
endif()
if(NOT MO2_UIBASE_LIB)
  message(FATAL_ERROR "Please set MO2_UIBASE_LIB")
endif()

# Set global include directories for Qt's moc preprocessor
include_directories(${MO2_UIBASE_INCLUDE})
include_directories(${MO2_SRC_PATH})
include_directories(${MO2_UIBASE_PATH}/game_features)
include_directories(${MO2_UIBASE_PATH})
include_directories(${MO2_SRC_PATH}/uibase/src)

# Build xngine base library
add_subdirectory(src/xngine)

# Build each game module
add_subdirectory(src/games/redguard)
add_subdirectory(src/games/daggerfall)
add_subdirectory(src/games/battlespire)
add_subdirectory(src/games/arena)
