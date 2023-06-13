# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

project(dwayland)

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui WaylandClient XkbCommonSupport)

add_definitions(-DQT5DWAYLANDPLUGIN_LIBRARY -DQT_DEPRECATED_WARNINGS)

file(GLOB GLOBAL_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/*.h
    ${CMAKE_SOURCE_DIR}/src/global.h
    ${CMAKE_SOURCE_DIR}/src/vtablehook.h
    ${CMAKE_SOURCE_DIR}/src/dxcbxsettings.h
    ${CMAKE_SOURCE_DIR}/src/dplatformsettings.h
    ${CMAKE_SOURCE_DIR}/src/dnativesettings.h
)
file(GLOB GLOBAL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_SOURCE_DIR}/src/global.cpp
    ${CMAKE_SOURCE_DIR}/src/vtablehook.cpp
    ${CMAKE_SOURCE_DIR}/src/dxcbxsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/dplatformsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/dnativesettings.cpp
)

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/plugins/platforms)

add_library(${PROJECT_NAME} MODULE main.cpp ${GLOBAL_SOURCES} ${GLOBAL_HEADERS})

target_compile_definitions(${PROJECT_NAME} 
PRIVATE
    QT5DWAYLANDPLUGIN_LIBRARY
    QT_DEPRECATED_WARNINGS
)

target_include_directories(${PROJECT_NAME}
PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    Qt5::Core
    Qt5::GuiPrivate
    Qt5::XkbCommonSupportPrivate
    Qt5::WaylandClientPrivate
)

if(NOT INSTALL_PATH)
    set(INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/platforms)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
