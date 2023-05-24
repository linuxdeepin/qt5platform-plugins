project(dxcb)

find_package(Qt${DTK_VERSION} REQUIRED COMPONENTS Core Widgets DBus)
if(DTK_VERSION STREQUAL "5")
    find_package(Qt${DTK_VERSION} REQUIRED COMPONENTS X11Extras)
endif()

# NOTE(sbw): 禁止语法树上的 vrp 优化，-O2/-O3 默认开启，会导致测试虚析构函数 HOOK 失败
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXXFLAGS_RELEASE} -fno-tree-vrp")

add_definitions(
    -DXDG_ICON_VERSION_MAR=${qt${QT_VERSION_MAJOR}xdgiconloader_VERSION_MAJOR})
add_definitions(-DDXCB_VERSION=\"${VERSION}\")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT=True)
endif()

file(GLOB GLOBAL_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB GLOBAL_SOURCES ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/3rdparty/*.c )
file(GLOB RCC_QRCS ${CMAKE_CURRENT_LIST_DIR}/cursors/cursor.qrc)
add_definitions(-DUSE_NEW_IMPLEMENTING)
set(DEFINED_DUSE_NEW_IMPLEMENTING TRUE)
if (DEFINED_DUSE_NEW_IMPLEMENTING)
    list(REMOVE_ITEM GLOBAL_HEADERS
        ${CMAKE_CURRENT_LIST_DIR}/dplatformbackingstore.h
        ${CMAKE_CURRENT_LIST_DIR}/dplatformwindowhook.h)
    list(REMOVE_ITEM GLOBAL_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/dplatformbackingstore.cpp
        ${CMAKE_CURRENT_LIST_DIR}/dplatformwindowhook.cpp)
endif()

include(${CMAKE_SOURCE_DIR}/src/src.cmake)

if(DTK_VERSION STREQUAL "5")
    qt5_add_dbus_interface(DBUS_INTERFACE_XMLS ../misc/com.deepin.im.xml
        im_interface)
    qt5_add_dbus_interface(DBUS_INTERFACE_XMLS ../misc/org.freedesktop.DBus.xml
        dbus_interface)
endif()

if(DTK_VERSION STREQUAL "6")
    qt_add_dbus_interface(DBUS_INTERFACE_XMLS ../misc/com.deepin.im.xml
        im_interface)
    qt_add_dbus_interface(DBUS_INTERFACE_XMLS ../misc/org.freedesktop.DBus.xml
        dbus_interface)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/plugins/platforms)

add_library(${PROJECT_NAME} MODULE ${GLOBAL_SOURCES} ${GLOBAL_HEADERS} ${RCC_QRCS} ${DBUS_INTERFACE_XMLS})

include(linux.cmake)

target_include_directories(${PROJECT_NAME}
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${Qt${DTK_VERSION}Core_PRIVATE_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

set(COMMON_LIBS Qt${DTK_VERSION}::Core Qt${DTK_VERSION}::Widgets)
if(DTK_VERSION STREQUAL "5")
    list(APPEND COMMON_LIBS Qt${DTK_VERSION}::X11Extras)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE ${COMMON_LIBS})

if(DTK_VERSION STREQUAL "5")
    find_package(Qt5 REQUIRED COMPONENTS Gui EdidSupport XkbCommonSupport)
    target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${Qt5EdidSupport_PRIVATE_INCLUDE_DIRS}
        ${Qt5XkbCommonSupport_PRIVATE_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Gui Qt5::EdidSupport Qt5::XkbCommonSupport)
endif()

if(DTK_VERSION STREQUAL "6")
    find_package(Qt6 REQUIRED COMPONENTS Gui OpenGL)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Gui Qt6::OpenGL)
endif()

if(NOT INSTALL_PATH)
    set(INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/qt${DTK_VERSION}/plugins/platforms)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
