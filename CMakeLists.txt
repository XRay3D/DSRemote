cmake_minimum_required(VERSION 3.5)

project(DSRemote VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES #[[Qt6]] Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)

add_compile_options(
    -Wextra
    -Wshadow
    -Wformat-nonliteral
    -Wformat-security
    -Wtype-limits
    -Wfatal-errors
    # $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    # $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
    # $<$<COMPILE_LANGUAGE:CXX>:-Wformat-nonliteral>
    # $<$<COMPILE_LANGUAGE:CXX>:-Wformat-security>
    # $<$<COMPILE_LANGUAGE:CXX>:-Wtype-limits>
    # $<$<COMPILE_LANGUAGE:CXX>:-Wfatal-errors>
)

# file(GLOB_RECURSE PROJECT_SOURCES *.ui *.cpp *.h *.qrc)

set(PROJECT_SOURCES
    images.qrc

    about_dialog.cpp
    about_dialog.h
    connection.cpp
    connection.h
    decode_dialog.cpp
    decode_dialog.h
    edflib.c
    edflib.h
    global.h
    interface.cpp
    lan_connect_thread.cpp
    lan_connect_thread.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow_constr.cpp
    playback_dialog.cpp
    playback_dialog.h
    read_settings_thread.cpp
    read_settings_thread.h
    save_data.cpp
    save_data_thread.cpp
    save_data_thread.h
    screen_thread.cpp
    screen_thread.h
    serial_decoder.cpp
    settings_dialog.cpp
    settings_dialog.h
    signalcurve.cpp
    signalcurve.h
    dial.cpp
    dial.h
    third_party/kiss_fft/_kiss_fft_guts.h
    third_party/kiss_fft/kiss_fft.c
    third_party/kiss_fft/kiss_fft.h
    third_party/kiss_fft/kiss_fftr.c
    third_party/kiss_fft/kiss_fftr.h
    timer_handlers.cpp
    tled.cpp
    tled.h
    tmc_dev.c
    tmc_dev.h
    # tmc_lan.c
    # tmc_lan.h
    tmclan.cpp
    tmclan.h
    utils.c
    utils.h
    wave_dialog.cpp
    wave_dialog.h
    wave_view.cpp
    wave_view.h

    mainwindow.ui
    mainwindow_raw.ui

    controlpanel.h
    controlpanel.cpp
    controlpanel.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DSRemote
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET DSRemote APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(DSRemote SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(DSRemote
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(DSRemote PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.DSRemote)
endif()
set_target_properties(DSRemote PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS DSRemote
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(DSRemote)
endif()
