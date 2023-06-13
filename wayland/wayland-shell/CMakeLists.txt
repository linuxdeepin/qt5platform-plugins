# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

project(kwayland-shell)

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets XkbCommonSupport WaylandClient)

include(wayland-shell.cmake)

try_compile(DEEPIN_KWIN_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR}/deepin-kwin-test
    ${CMAKE_CURRENT_LIST_DIR}/config.tests/deepin-kwin-test deepin-kwin-test)

try_compile(WAYLAND_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR}/wayland_test
    ${CMAKE_CURRENT_LIST_DIR}/config.tests/wayland_test wayland_test)

if(WAYLAND_TEST_COMPILE_RESULT)
    add_definitions(-DUSE_DEEPIN_WAYLAND)
    set(GLOBAL_HEADERS
        dwaylandshellmanager.h
        dkeyboard.h
        ${CMAKE_SOURCE_DIR}/src/global.h
        ${CMAKE_SOURCE_DIR}/src/vtablehook.h
    )
    set(GLOBAL_SOURCES
        dwaylandshellmanager.cpp
        dkeyboard.cpp
        ${CMAKE_SOURCE_DIR}/src/global.cpp
        ${CMAKE_SOURCE_DIR}/src/vtablehook.cpp
    )
else()
    message(WARNING "QtWayland version incompatible")
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/plugins/wayland-shell-integration)

add_library(${PROJECT_NAME} MODULE main.cpp ${GLOBAL_SOURCES} ${GLOBAL_HEADERS})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    QT5DWAYLANDPLUGIN_LIBRARY
    QT_DEPRECATED_WARNINGS
)

if(DEEPIN_KWIN_TEST_COMPILE_RESULT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE D_DEEPIN_KWIN)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

list(APPEND COMMON_LIBS
    Qt5::Core
    Qt5::Gui
    Qt5::GuiPrivate
    Qt5::Widgets
    Qt5::WidgetsPrivate
    Qt5::WaylandClient
    Qt5::WaylandClientPrivate
    Qt5::XkbCommonSupportPrivate
)
target_link_libraries(${PROJECT_NAME} ${COMMON_LIBS})

if(NOT INSTALL_PATH)
    set(INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/wayland-shell-integration)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
