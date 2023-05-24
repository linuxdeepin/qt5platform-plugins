project(dwayland)

find_package(Qt${DTK_VERSION} REQUIRED COMPONENTS Core WaylandClient)
if(DTK_VERSION STREQUAL "5")
  find_package(Qt5 REQUIRED COMPONENTS XkbCommonSupport)
endif()

set(GLOBAL_HEADERS
    dwaylandinterfacehook.h
    dwaylandintegration.h
    dnotitlebarwindowhelper_wl.h
    dhighdpi.h
    dxsettings.h
    ${CMAKE_SOURCE_DIR}/src/global.h
    ${CMAKE_SOURCE_DIR}/src/vtablehook.h
    ${CMAKE_SOURCE_DIR}/src/dxcbxsettings.h
    ${CMAKE_SOURCE_DIR}/src/dplatformsettings.h
    ${CMAKE_SOURCE_DIR}/src/dnativesettings.h)
set(GLOBAL_SOURCES
    dwaylandinterfacehook.cpp
    dwaylandintegration.cpp
    dnotitlebarwindowhelper_wl.cpp
    dhighdpi.cpp
    dxsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/global.cpp
    ${CMAKE_SOURCE_DIR}/src/vtablehook.cpp
    ${CMAKE_SOURCE_DIR}/src/dxcbxsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/dplatformsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/dnativesettings.cpp)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/plugins/platforms)

add_library(${PROJECT_NAME} MODULE main.cpp ${GLOBAL_SOURCES} ${GLOBAL_HEADERS})

target_compile_definitions(${PROJECT_NAME} PRIVATE QT5DWAYLANDPLUGIN_LIBRARY
                                                   QT_DEPRECATED_WARNINGS)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${Qt${DTK_VERSION}Gui_PRIVATE_INCLUDE_DIRS}
         ${Qt${DTK_VERSION}WaylandClient_PRIVATE_INCLUDE_DIRS}
         ${Qt${DTK_VERSION}WaylandGlobalPrivate_INCLUDE_DIRS}
         ${CMAKE_SOURCE_DIR}/src)

if(DTK_VERSION STREQUAL "6")
  list(GET Qt6WaylandClient_PRIVATE_INCLUDE_DIRS 0 dir)
  if(EXISTS ${dir})
    include_directories(${Qt6WaylandClient_PRIVATE_INCLUDE_DIRS})
  else()
    include_directories(
      ${CMAKE_CURRENT_LIST_DIR}/../qtwayland-dev/${Qt6_VERSION})
  endif()
endif()

if(DTK_VERSION STREQUAL "5")
  target_include_directories(
    ${PROJECT_NAME}
    PUBLIC ${Qt${DTK_VERSION}XkbCommonSupport_PRIVATE_INCLUDE_DIRS})
endif()

target_link_libraries(
  ${PROJECT_NAME} PUBLIC Qt${DTK_VERSION}::Core
                         ${Qt${DTK_VERSION}WaylandClient_LIBRARIES})

if(NOT INSTALL_PATH)
  set(INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/qt${DTK_VERSION}/plugins/platforms)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
