find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# Create xngine static library
add_library(xngine STATIC
	dummybsa.cpp
	dummybsa.h
	gamexngine.cpp
	gamexngine.h
	xnginebsainvalidation.cpp
	xnginebsainvalidation.h
	xnginedataarchives.cpp
	xnginedataarchives.h
	xnginegameplugins.cpp
	xnginegameplugins.h
	xnginelocalsavegames.cpp
	xnginelocalsavegames.h
	xnginemoddatachecker.cpp
	xnginemoddatachecker.h
	xnginemoddatacontent.cpp
	xnginemoddatacontent.h
	xnginesavegame.cpp
	xnginesavegame.h
	xnginesavegameinfo.cpp
	xnginesavegameinfo.h
	xnginesavegameinfowidget.cpp
	xnginesavegameinfowidget.h
	xnginesavegameinfowidget.ui
	xnginescriptextender.cpp
	xnginescriptextender.h
	xngineunmanagedmods.cpp
	xngineunmanagedmods.h
	vdf_parser.h
)

# Set AUTOMOC properties BEFORE target_include_directories
set_target_properties(xngine PROPERTIES
	AUTOMOC ON
	AUTOUIC ON
)

target_include_directories(xngine PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${MO2_UIBASE_INCLUDE}
	${MO2_UIBASE_PATH}/include/uibase/game_features
	${MO2_SRC_PATH}
)

# Configure MOC options for AUTOMOC
set_property(TARGET xngine APPEND PROPERTY AUTOMOC_MOC_OPTIONS
	"-I${MO2_UIBASE_INCLUDE}"
	"-I${MO2_SRC_PATH}"
	"-I${MO2_UIBASE_PATH}/include/uibase/game_features"
)

# Set CMAKE_AUTOMOC_MOC_OPTIONS to include additional paths for moc preprocessor
set(CMAKE_AUTOMOC_MOC_OPTIONS "-I${MO2_UIBASE_PATH}")
list(APPEND CMAKE_AUTOMOC_MOC_OPTIONS "-I${MO2_SRC_PATH}")
list(APPEND CMAKE_AUTOMOC_MOC_OPTIONS "-I${MO2_UIBASE_PATH}/game_features")

# Try to find compression libraries; they may be provided by vcpkg
find_package(ZLIB QUIET)
find_package(lz4 CONFIG QUIET)

target_link_libraries(xngine PUBLIC
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	${MO2_UIBASE_LIB}
	imagehlp.lib
)

# Link against compression libraries if found
if(ZLIB_FOUND)
	target_link_libraries(xngine PUBLIC ZLIB::ZLIB)
endif()

if(lz4_FOUND)
	target_link_libraries(xngine PUBLIC lz4::lz4)
endif()

set_target_properties(xngine PROPERTIES
	AUTOMOC ON
	AUTOUIC ON
)
