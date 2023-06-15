# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.13)

set(DTK_VERSION
    "5.6.8"
    CACHE STRING "define project version"
)

project(qt5platform-plugins
    VERSION ${DTK_VERSION}
    DESCRIPTION "DTK platform plugin module"
    HOMEPAGE_URL "https://github.com/linuxdeepin/qt5platform-plugins"
    LANGUAGES CXX C
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if("${PROJECT_VERSION_MAJOR}" STREQUAL "5")
    set(QT_VERSION_MAJOR "5")
elseif("${PROJECT_VERSION_MAJOR}" STREQUAL "6")
    set(QT_VERSION_MAJOR "6")
    set(DTK_VERSION_MAJOR "6")
else()
    message(SEND_ERROR "not support Prject Version ${PROJECT_VERSION}.")
endif()
message(${PROJECT_VERSION_MAJOR})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REQUIRED_QT_VERSION 5.15.0)

add_subdirectory(xcb)
if("${PROJECT_VERSION_MAJOR}" STREQUAL "5")
    add_subdirectory(wayland)
endif()
message(${PROJECT_VERSION_MAJOR})
