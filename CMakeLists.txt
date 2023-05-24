cmake_minimum_required(VERSION 3.10)

set(VERSION
    "6.0.0"
    CACHE STRING "define project version")
set(DTK_VERSION "5")

project(
  qt5platform-plugins
  VERSION ${VERSION}
  DESCRIPTION "DTK Plugin module"
  HOMEPAGE_URL "https://github.com/linuxdeepin/qt5platform-plugins"
  LANGUAGES CXX C)

option(ENABLE_QT5 "Enable Qt 5" ON)
option(ENABLE_QT6 "Enable Qt 6" ON)

if(ENABLE_QT5)
  set(DTK_VERSION "5")
endif()

if(ENABLE_QT6)
  set(DTK_VERSION "6")
endif()

message(DtkVersion: ${DTK_VERSION})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(xcb)
add_subdirectory(wayland)
