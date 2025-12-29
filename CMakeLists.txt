# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.13)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" FILE_VERSION)
string(STRIP "${FILE_VERSION}" FILE_VERSION)

project(qt5platform-plugins
    VERSION ${FILE_VERSION}
    DESCRIPTION "DTK platform plugin module"
    HOMEPAGE_URL "https://github.com/linuxdeepin/qt5platform-plugins"
    LANGUAGES CXX C
)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(DTK5 "Build DTK5." ON)
if(DTK5)
    set(DTK_VERSION_MAJOR "5")
else()
    set(DTK_VERSION_MAJOR "6")
endif()

set(DTK_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(DTK_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(DTK_VERSION "${DTK_VERSION_MAJOR}.${DTK_VERSION_MINOR}.${DTK_VERSION_PATCH}")
set(QT_VERSION_MAJOR ${DTK_VERSION_MAJOR})

message(STATUS "Building DTK${DTK_VERSION_MAJOR} (Qt${QT_VERSION_MAJOR}) version: ${DTK_VERSION}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(REQUIRED_QT_VERSION 5.15.0)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_TESTING ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fno-omit-frame-pointer")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif ()
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugins/platforms)

add_subdirectory(xcb)
if(DTK5)
    add_subdirectory(wayland)
endif()
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
