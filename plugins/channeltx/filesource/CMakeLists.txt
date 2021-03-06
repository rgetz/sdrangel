project(filesource)

if (HAS_SSSE3)
    message(STATUS "RemoteSource: use SSSE3 SIMD" )
elseif (HAS_NEON)
    message(STATUS "RemoteSource: use Neon SIMD" )
else()
    message(STATUS "RemoteSource: Unsupported architecture")
    return()
endif()

set(filesource_SOURCES
	filesource.cpp
	filesourceplugin.cpp
	filesourcesettings.cpp
)

set(filesource_HEADERS
	filesource.h
	filesourceplugin.h
	filesourcesettings.h
)

include_directories(
    ${CMAKE_SOURCE_DIR}/swagger/sdrangel/code/qt5/client
    ${CUSTOM_WINDOWS_INCLUDE}
)

if(NOT SERVER_MODE)
    set(filesource_SOURCES
        ${filesource_SOURCES}
        filesourcegui.cpp
        filesourcegui.ui
    )
    set(filesource_HEADERS
        ${filesource_HEADERS}
        filesourcegui.h
    )
    set(TARGET_NAME filesource)
    set(TARGET_LIB "Qt5::Widgets")
    set(TARGET_LIB_GUI "sdrgui")
    set(INSTALL_FOLDER ${INSTALL_PLUGINS_DIR})
else()
    set(TARGET_NAME filesourcesrv)
    set(TARGET_LIB "")
    set(TARGET_LIB_GUI "")
    set(INSTALL_FOLDER ${INSTALL_PLUGINSSRV_DIR})
endif()

add_library(${TARGET_NAME} SHARED
	${filesource_SOURCES}
)

target_link_libraries(${TARGET_NAME}
    Qt5::Core
    ${TARGET_LIB}
    Qt5::Network
	sdrbase
	${TARGET_LIB_GUI}
    swagger
)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_FOLDER})
