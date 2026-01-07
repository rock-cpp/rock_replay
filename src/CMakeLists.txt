include(RockQt)

rock_find_qt4(OPTIONAL)
rock_find_qt5(OPTIONAL Widgets)

if(ROCK_QT_VERSION_4)
    QT4_ADD_RESOURCES(VIZ_RESOURCES_QT4 ressources.qrc)
endif()
if(ROCK_QT_VERSION_5)
    qt5_add_resources(VIZ_RESOURCES_QT5 ressources.qrc)
endif()

rock_library(rock_replay
    SOURCES
        ReplayHandler.cpp
        LogTask.cpp
        LogTaskManager.cpp
        LogFileHelper.cpp
    HEADERS
        ReplayHandler.hpp
        LogTask.hpp
        LogTaskManager.hpp
        LogFileHelper.hpp
    DEPS_PKGCONFIG
        base-logging
        orocos_cpp
        pocolog_cpp
    LIBS
        gcov
)

rock_library(arg_parser
    SOURCES
        ArgParser.cpp
    HEADERS
        ArgParser.hpp
    LIBS
        boost_program_options
        gcov
)

set(ROCK_REPLAY2_TARGET_QT5 "rock-replay2-qt5")
if(ROCK_QT_VERSION_4)
    if(ROCK_QT_VERSION_5)
        message(STATUS "Building Qt5 rock-replay as rock-replay2-qt5")
    endif()
else()
    if(ROCK_QT_VERSION_5)
        set(ROCK_REPLAY2_TARGET_QT5 "rock-replay2")
    else()
        message(FATAL "Qt not found")
    endif()
endif()

rock_qt_executable(
    TARGETPREFIX rock-replay2
    QT4_SUFFIX ""
    TARGET_QT5 ${ROCK_REPLAY2_TARGET_QT5}
    MISSINGQTDEPS_NOBUILD
    SOURCES
        Main.cpp
        ReplayGui.cpp
    SOURCES_QT4
        ${VIZ_RESOURCES_QT4}
    SOURCES_QT5
        ${VIZ_RESOURCES_QT5}
    MOC
        ReplayGui.h
    UI
        main.ui
    DEPS
        rock_replay
        arg_parser
    DEPS_QT5
        Qt5::Widgets
)
