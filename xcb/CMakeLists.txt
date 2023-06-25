# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

project(dxcb)

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets DBus)
if(${QT_VERSION_MAJOR} STREQUAL "5")
    find_package(Qt5 REQUIRED COMPONENTS XcbQpa X11Extras EdidSupport XkbCommonSupport)
else()
    find_package(Qt6 REQUIRED COMPONENTS OpenGL XcbQpaPrivate)
endif()

# NOTE(sbw): 禁止语法树上的 vrp 优化，-O2/-O3 默认开启，会导致测试虚析构函数 HOOK 失败
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-tree-vrp" CACHE STRING "disable vrp optimization" FORCE)

add_definitions(-DDXCB_VERSION=\"${DTK_VERSION}\")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT=TRUE)
endif()
file(GLOB GLOBAL_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB GLOBAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/3rdparty/*.c)
file(GLOB RCC_QRCS ${CMAKE_CURRENT_LIST_DIR}/cursors/cursor.qrc)
add_definitions(-DUSE_NEW_IMPLEMENTING)
set(USE_NEW_IMPLEMENTING TRUE)
if (USE_NEW_IMPLEMENTING)
    list(REMOVE_ITEM GLOBAL_HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/dplatformbackingstore.h
        ${CMAKE_CURRENT_LIST_DIR}/dplatformwindowhook.h)
    list(REMOVE_ITEM GLOBAL_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/dplatformbackingstore.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dplatformwindowhook.cpp)
endif()

include(${CMAKE_SOURCE_DIR}/src/src.cmake)

if(${QT_VERSION_MAJOR} STREQUAL "5")
    qt5_add_dbus_interface(DBUS_INTERFACE_XMLS ${CMAKE_SOURCE_DIR}/misc/com.deepin.im.xml im_interface)
    qt5_add_dbus_interface(DBUS_INTERFACE_XMLS ${CMAKE_SOURCE_DIR}/misc/org.freedesktop.DBus.xml dbus_interface)
else()
    qt6_add_dbus_interface(DBUS_INTERFACE_XMLS ${CMAKE_SOURCE_DIR}/misc/com.deepin.im.xml im_interface)
    qt6_add_dbus_interface(DBUS_INTERFACE_XMLS ${CMAKE_SOURCE_DIR}/misc/org.freedesktop.DBus.xml dbus_interface)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/plugins/platforms)

add_library(${PROJECT_NAME} MODULE ${GLOBAL_SOURCES} ${GLOBAL_HEADERS} ${RCC_QRCS} ${DBUS_INTERFACE_XMLS})

include(${CMAKE_CURRENT_LIST_DIR}/linux.cmake)

target_link_libraries(${PROJECT_NAME}
PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::CorePrivate
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::Widgets
)
if(${QT_VERSION_MAJOR} STREQUAL "5")
    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt5::XcbQpa
        Qt5::EdidSupport
        Qt5::EdidSupportPrivate
        Qt5::XkbCommonSupport
        Qt5::XkbCommonSupportPrivate
        Qt5::X11Extras
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::OpenGL Qt6::OpenGLPrivate Qt6::XcbQpaPrivate)
endif()

if(NOT INSTALL_PATH)
    set(INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/qt${QT_VERSION_MAJOR}/plugins/platforms)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
