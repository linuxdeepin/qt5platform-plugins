# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.10)

set(VERSION
    "5.6.8"
    CACHE STRING "define project version"
)

project(qt5platform-plugins
    VERSION ${VERSION}
    DESCRIPTION "DTK platform plugin module"
    HOMEPAGE_URL "https://github.com/linuxdeepin/qt5platform-plugins"
    LANGUAGES CXX C
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REQUIRED_QT_VERSION 5.15.0)

add_subdirectory(xcb)
add_subdirectory(wayland)
