project(kwayland-shell)

find_package(Qt${DTK_VERSION} REQUIRED COMPONENTS Core WaylandClient)
if(DTK_VERSION STREQUAL "5")
  find_package(Qt5 REQUIRED COMPONENTS XkbCommonSupport)
endif()
if(DTK_VERSION STREQUAL "6")
  find_package(Qt6 REQUIRED COMPONENTS WaylandCompositor)
endif()
find_package(DWayland REQUIRED)

try_compile(
  DEEPIN_KWIN_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR}/deepin-kwin-test
  ${CMAKE_CURRENT_LIST_DIR}/config.tests/deepin-kwin-test deepin-kwin-test)

try_compile(WAYLAND_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR}/wayland_test
            ${CMAKE_CURRENT_LIST_DIR}/config.tests/wayland_test wayland_test)

if(WAYLAND_TEST_COMPILE_RESULT)
  add_definitions(-DDTHEMED_ICON_LOOKUP)
  set(GLOBAL_HEADERS dwaylandshellmanager.cpp dkeyboard.cpp
                     ../../src/global.cpp ../../src/vtablehook.cpp)
  set(GLOBAL_SOURCES dwaylandshellmanager.cpp dkeyboard.cpp
                     ../../src/global.cpp ../../src/vtablehook.cpp)
else()
  message(WARNING "QtWayland version incompatible")
endif()

set(LIBRARY_OUTPUT_PATH
    ${CMAKE_SOURCE_DIR}/bin/plugins/wayland-shell-integration)
add_library(${PROJECT_NAME} MODULE main.cpp ${GLOBAL_SOURCES} ${GLOBAL_HEADERS})

# Qt >= 5.8
if(DTK_VERSION STREQUAL "5")
  if(${Qt5_VERSION_MINOR} GREATER 7)
    find_package(Qt5 REQUIRED Gui Widgets)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Gui Qt5::Widgets)
    target_include_directories(
      ${PROJECT_NAME} PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
                              ${Qt5Widgets_PRIVATE_INCLUDE_DIRS})
  else()
    find_package(Qt5 REQUIRED PlatformSupport)
  endif()
endif()

if(DTK_VERSION STREQUAL "6")
  find_package(Qt6 REQUIRED Gui Widgets)
  target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Gui Qt6::Widgets)
  target_include_directories(
    ${PROJECT_NAME} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS}
                            ${Qt6Widgets_PRIVATE_INCLUDE_DIRS})
endif()

target_compile_definitions(
  ${PROJECT_NAME} PRIVATE QT5DWAYLANDPLUGIN_LIBRARY QT_DEPRECATED_WARNINGS
                          D_DEEPIN_IS_DWAYLAND)
if(DEEPIN_KWIN_TEST_COMPILE_RESULT)
  target_compile_definitions(${PROJECT_NAME} PRIVATE D_DEEPIN_KWIN)
endif()

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${Qt${DTK_VERSION}WaylandClient_PRIVATE_INCLUDE_DIRS}
                          ../../src)

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
    ${PROJECT_NAME} PRIVATE ${Qt5XkbCommonSupport_PRIVATE_INCLUDE_DIRS})
endif()

target_link_libraries(
  ${PROJECT_NAME} PRIVATE Qt${DTK_VERSION}::Core
                          ${Qt${DTK_VERSION}WaylandClient_LIBRARIES})

if(DTK_VERSION STREQUAL "6")
  target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::WaylandCompositor)
endif()

if(NOT INSTALL_PATH)
  set(INSTALL_PATH
      ${CMAKE_INSTALL_LIBDIR}/qt${QT_VERSION_MAJOR}/plugins/wayland-shell-integration
  )
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
