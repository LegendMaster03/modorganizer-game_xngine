cmake_minimum_required(VERSION 3.16)

project(game_xngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt SDK path (override via -DQT_ROOT or -DCMAKE_PREFIX_PATH)
set(QT_ROOT "C:/Qt/6.7.1/msvc2019_64" CACHE PATH "Path to Qt SDK root")
if(QT_ROOT AND EXISTS "${QT_ROOT}")
  list(PREPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# MO2 SDK paths - Configure these paths to point to your MO2 development directories
# For GitHub-friendly configuration, these are defined with defaults that can be overridden
# Paths can be relative or absolute; only define here (not in subdirectories)
set(MO2_UIBASE_PATH "C:/Users/kbarn/Downloads/modorganizer-uibase-4885d450d84a978d61b104ef82eb130f394c7f78/modorganizer-uibase-4885d450d84a978d61b104ef82eb130f394c7f78" CACHE PATH "Path to MO2 uibase source directory")
set(MO2_UIBASE_INCLUDE "${MO2_UIBASE_PATH}/include/uibase" CACHE PATH "Path to MO2 uibase include directory")
set(MO2_SRC_PATH "C:/Users/kbarn/Downloads/modorganizer-master/modorganizer-master" CACHE PATH "Path to MO2 source directory")
set(MO2_UIBASE_LIB "C:/Users/kbarn/Downloads/Mod.Organizer-2.5.2-uibase/uibase.lib" CACHE PATH "Path to MO2 uibase.lib")

if(NOT MO2_UIBASE_PATH OR NOT MO2_SRC_PATH)
  message(FATAL_ERROR "Please set MO2_UIBASE_PATH and MO2_SRC_PATH CMake variables")
endif()

# Set global include directories for Qt's moc preprocessor
include_directories(${MO2_UIBASE_INCLUDE})
include_directories(${MO2_SRC_PATH})
include_directories(${MO2_UIBASE_PATH}/include/uibase/game_features)

# Export paths to subdirectories
set(MO2_UIBASE_PATH ${MO2_UIBASE_PATH} PARENT_SCOPE)
set(MO2_UIBASE_INCLUDE ${MO2_UIBASE_INCLUDE} PARENT_SCOPE)
set(MO2_SRC_PATH ${MO2_SRC_PATH} PARENT_SCOPE)
set(MO2_UIBASE_LIB ${MO2_UIBASE_LIB} PARENT_SCOPE)

# Build xngine base library
add_subdirectory(src/xngine)

# Build each game module
add_subdirectory(src/games/redguard)
add_subdirectory(src/games/daggerfall)
add_subdirectory(src/games/battlespire)
