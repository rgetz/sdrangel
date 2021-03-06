project(freqtracker)

set(freqtracker_SOURCES
	freqtracker.cpp
	freqtrackersettings.cpp
	freqtrackerplugin.cpp
)

set(freqtracker_HEADERS
	freqtracker.h
	freqtrackersettings.h
	freqtrackerplugin.h
)

include_directories(
        ${CMAKE_SOURCE_DIR}/swagger/sdrangel/code/qt5/client
)

if(NOT SERVER_MODE)
    set(freqtracker_SOURCES
        ${freqtracker_SOURCES}
        freqtrackergui.cpp

	freqtrackergui.ui
    )
    set(freqtracker_HEADERS
        ${freqtracker_HEADERS}
        freqtrackergui.h
    )

    set(TARGET_NAME freqtracker)
    set(TARGET_LIB "Qt5::Widgets")
    set(TARGET_LIB_GUI "sdrgui")
    set(INSTALL_FOLDER ${INSTALL_PLUGINS_DIR})
else()
    set(TARGET_NAME freqtrackersrv)
    set(TARGET_LIB "")
    set(TARGET_LIB_GUI "")
    set(INSTALL_FOLDER ${INSTALL_PLUGINSSRV_DIR})
endif()

add_library(${TARGET_NAME} SHARED
	${freqtracker_SOURCES}
)

target_link_libraries(${TARGET_NAME}
        Qt5::Core
        ${TARGET_LIB}
	sdrbase
	${TARGET_LIB_GUI}
)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_FOLDER})
