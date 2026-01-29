cmake_minimum_required(VERSION 3.16)
project(game_redguard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Force Windows threading (avoid pthread probes on MSVC)
set(CMAKE_USE_WIN32_THREADS_INIT ON CACHE BOOL "" FORCE)
set(CMAKE_USE_PTHREADS_INIT OFF CACHE BOOL "" FORCE)
set(CMAKE_THREAD_PREFER_PTHREAD OFF CACHE BOOL "" FORCE)
set(THREADS_PREFER_PTHREAD_FLAG OFF CACHE BOOL "" FORCE)
set(CMAKE_HAVE_LIBC_PTHREAD FALSE CACHE BOOL "" FORCE)
set(CMAKE_HAVE_PTHREADS_CREATE FALSE CACHE BOOL "" FORCE)
set(CMAKE_HAVE_PTHREAD_CREATE FALSE CACHE BOOL "" FORCE)
set(CMAKE_HAVE_THREADS_LIBRARY TRUE CACHE BOOL "" FORCE)
set(CMAKE_THREAD_LIBS_INIT "" CACHE STRING "" FORCE)
set(Threads_FOUND TRUE CACHE BOOL "" FORCE)

# Force Qt 6.7.1 to match MO2's Qt version
set(CMAKE_PREFIX_PATH "C:/Qt/6.7.1/msvc2019_64" CACHE PATH "Qt installation path" FORCE)

# Find Qt (prefer Qt6, fallback to Qt5)
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt6_FOUND)
    set(QT_LIB Qt6)
else()
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    set(QT_LIB Qt5)
endif()

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files for the game plugin
# Core data-handling files (no MO2 API dependency)
set(SOURCE_FILES
    src/RtxDatabase.cpp
    src/MenuFile.cpp
    src/MapChanges.cpp
    src/MapFile.cpp
    src/MapHeader.cpp
    src/ScriptInstruction.cpp
    src/Utils.cpp
    src/gameredguard.cpp
    src/redguardsavegame.cpp
    src/RGMODFrameworkWrapper.cpp
    src/ModLoader.cpp
    src/RedguardDataChecker.cpp
)

# Header files
set(HEADER_FILES
    src/RGMODFrameworkWrapper.h
    src/ModLoader.h
    src/RtxDatabase.h
    src/MenuFile.h
    src/MapChanges.h
    src/MapFile.h
    src/MapHeader.h
    src/ScriptInstruction.h
    src/gameRedguard.h
    src/redguardsavegame.h
    src/Utils.h
    src/RedguardDataChecker.h
)

# Create the plugin library
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${QT_LIB}::Core ${QT_LIB}::Gui ${QT_LIB}::Widgets)

# Link MO2 API (make path configurable)
# Default to the provided uibase build and full source archive paths. You can override via -DMO2_SDK_DIR or -DMO2_SRC_DIR.
set(MO2_SDK_DIR "C:/Users/kbarn/Downloads/Mod.Organizer-2.5.2-uibase" CACHE PATH "Path to MO2 install or SDK root")
set(MO2_SRC_DIR "C:/Users/kbarn/Downloads/Mod.Organizer-2.5.2-src" CACHE PATH "Path to MO2 full source root")

set(MO2_INC_DIR "C:/Users/kbarn/Downloads/Mod.Organizer-2.5.2-uibase" CACHE PATH "Override MO2 include directory")
set(MO2_LIB_DIR "" CACHE PATH "Override MO2 library directory")

if(NOT MO2_INC_DIR)
    if(EXISTS "${MO2_SDK_DIR}/include")
        set(MO2_INC_DIR "${MO2_SDK_DIR}/include")
    elseif(EXISTS "${MO2_SDK_DIR}/src/api")
        set(MO2_INC_DIR "${MO2_SDK_DIR}/src/api")
    elseif(EXISTS "${MO2_SDK_DIR}/uibase/src")
        # MO2 source archive layout (headers like iplugingame.h live here)
        set(MO2_INC_DIR "${MO2_SDK_DIR}/uibase/src")
    elseif(EXISTS "${MO2_SRC_DIR}/include")
        set(MO2_INC_DIR "${MO2_SRC_DIR}/include")
    elseif(EXISTS "${MO2_SRC_DIR}/src/api")
        set(MO2_INC_DIR "${MO2_SRC_DIR}/src/api")
    elseif(EXISTS "${MO2_SRC_DIR}/uibase/src")
        set(MO2_INC_DIR "${MO2_SRC_DIR}/uibase/src")
    endif()
endif()

if(NOT MO2_LIB_DIR)
    if(EXISTS "${MO2_SDK_DIR}/lib")
        set(MO2_LIB_DIR "${MO2_SDK_DIR}/lib")
    elseif(EXISTS "${MO2_SDK_DIR}/install/lib")
        set(MO2_LIB_DIR "${MO2_SDK_DIR}/install/lib")
    elseif(EXISTS "${MO2_SDK_DIR}/uibase/lib")
        # If you built uibase separately
        set(MO2_LIB_DIR "${MO2_SDK_DIR}/uibase/lib")
    elseif(EXISTS "${MO2_SDK_DIR}/build/uibase/lib")
        set(MO2_LIB_DIR "${MO2_SDK_DIR}/build/uibase/lib")
    elseif(EXISTS "${MO2_SRC_DIR}/lib")
        set(MO2_LIB_DIR "${MO2_SRC_DIR}/lib")
    elseif(EXISTS "${MO2_SRC_DIR}/uibase/lib")
        set(MO2_LIB_DIR "${MO2_SRC_DIR}/uibase/lib")
    elseif(EXISTS "${MO2_SRC_DIR}/build/uibase/lib")
        set(MO2_LIB_DIR "${MO2_SRC_DIR}/build/uibase/lib")
    elseif(EXISTS "${MO2_SRC_DIR}/install/lib")
        set(MO2_LIB_DIR "${MO2_SRC_DIR}/install/lib")
    endif()
endif()

if(MO2_INC_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE "${MO2_INC_DIR}")
    # Also add game_features subdirectory if it exists
    if(EXISTS "${MO2_INC_DIR}/game_features")
        target_include_directories(${PROJECT_NAME} PRIVATE "${MO2_INC_DIR}/game_features")
    endif()
endif()

if(MO2_LIB_DIR)
    target_link_directories(${PROJECT_NAME} PRIVATE "${MO2_LIB_DIR}")
endif()

find_library(MO2_UIBASE_LIB NAMES uibase PATHS "${MO2_LIB_DIR}" "C:/Users/kbarn/Downloads/Mod.Organizer-2.5.2-uibase" NO_DEFAULT_PATH)

if(MO2_UIBASE_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MO2_UIBASE_LIB})
endif()

# Set output directory for the DLL
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# Platform-specific settings
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
endif()
