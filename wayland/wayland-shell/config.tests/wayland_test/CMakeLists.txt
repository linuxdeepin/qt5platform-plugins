# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.10)

project(wayland_test)

set(REQUIRED_QT_VERSION 5.15.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui WaylandClient XkbCommonSupport)

include(../../wayland-shell.cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

add_executable(${PROJECT_NAME} main.cpp)

list(APPEND COMMON_LIBS
    Qt5::Core
    Qt5::CorePrivate
    Qt5::GuiPrivate
    Qt5::WaylandClientPrivate
    Qt5::XkbCommonSupportPrivate
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${COMMON_LIBS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)
