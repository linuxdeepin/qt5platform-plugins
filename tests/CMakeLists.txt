# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

project(ut-platformplugins)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Widgets Test)
find_package(GTest REQUIRED)
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

include(${CMAKE_SOURCE_DIR}/xcb/xcb.cmake)
include(${CMAKE_SOURCE_DIR}/src/src.cmake)

file(GLOB test_SRC test.h main.cpp src/*.cpp)

add_executable(${PROJECT_NAME} ${GLOBAL_HEADERS} ${GLOBAL_SOURCES} ${test_SRC} ${xcb_SRC})

include(${CMAKE_SOURCE_DIR}/xcb/linux.cmake)

target_compile_options(${PROJECT_NAME} PRIVATE -fno-access-control -fsanitize=address)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::Test
    GTest::GTest
    gmock
    pthread
    m
    gcov
)

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
