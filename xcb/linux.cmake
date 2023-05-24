find_package(PkgConfig REQUIRED)

pkg_check_modules(
    XCB
    REQUIRED IMPORTED_TARGET
    x11-xcb
    xi
    xcb-renderutil
    sm
    ice
    xcb-render
    dbus-1
    xcb
    xcb-image
    xcb-icccm
    xcb-sync
    xcb-xfixes
    xcb-shm
    xcb-randr
    xcb-shape
    xcb-keysyms
    xcb-xkb
    xcb-composite
    xkbcommon-x11
    xcb-damage
    xcb-xinerama
    mtdev
    egl
    cairo)

target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::XCB)

if(DTK_VERSION STREQUAL "5")
    find_package(Qt5 REQUIRED XcbQpa)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::XcbQpa)
endif()

if(DTK_VERSION STREQUAL "6")
    find_package(Qt6 REQUIRED XcbQpaPrivate)
endif()

if(DTK_VERSION STREQUAL "5")
    get_property(QT_ENABLED_PRIVATE_FEATURES TARGET Qt5::Gui PROPERTY QT_ENABLED_PRIVATE_FEATURES)
    list(FIND QT_ENABLED_PRIVATE_FEATURES "xcb-xlib" index)
    if (index GREATER 0)
        add_definitions(-DXCB_USE_XLIB -DXCB_USE_XINPUT2 -DXCB_USE_XINPUT21 -DXCB_USE_XINPUT22)
    endif()
# build with session management support
    list(FIND QT_ENABLED_PRIVATE_FEATURES "xcb-sm" index)
    if (index GREATER 0)
        add_definitions(-DXCB_USE_SM)
    endif()
    list(FIND QT_ENABLED_PRIVATE_FEATURES "xcb-qt" index)
    if (index GREATER 0)
        add_definitions(-DXCB_USE_RENDER)
    endif()
    list(FIND QT_ENABLED_PRIVATE_FEATURES "system-xcb" index)
    if (index GREATER 0)
        list(FIND QT_ENABLED_PRIVATE_FEATURES "xcb-render" index)
        if (index GREATER 0)
            add_definitions(-DXCB_USE_RENDER)
        endif()
        list(FIND QT_ENABLED_PRIVATE_FEATURES "xkb" index)
        if (index GREATER 0)
            add_definitions(-DXCB_USE_RENDER)
        endif()
    else()
        add_definitions(-DXCB_USE_RENDER)
    endif()
endif()

if(DTK_VERSION STREQUAL "6")
    get_property(QT_ENABLED_PRIVATE_FEATURES TARGET Qt6::Gui PROPERTY QT_ENABLED_PRIVATE_FEATURES)
    list(FIND QT_ENABLED_PRIVATE_FEATURES "xcb-xlib" index)
    if (index GREATER 0)
        add_definitions(-DXCB_USE_XLIB -DXCB_USE_XINPUT2 -DXCB_USE_XINPUT21 -DXCB_USE_XINPUT22)
    endif()
endif()

if(DTK_VERSION STREQUAL "5")
    list(GET Qt5Core_INCLUDE_DIRS 0 dir)
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/libqt5xcbqpa-dev/${Qt5_VERSION})
        include_directories(
            ${CMAKE_CURRENT_LIST_DIR}/libqt5xcbqpa-dev/${Qt5_VERSION})
    elseif(EXISTS ${dir}QtXcb/${Qt5_VERSION}/QtXcb/private)
        include_directories(${dir}QtXcb/${Qt5_VERSION}/QtXcb/private)
    else()
        message(FATAL_ERROR "Not support Qt Version: ${Qt5_VERSION}")
    endif()
endif()

if(DTK_VERSION STREQUAL "6")
    include_directories(${CMAKE_CURRENT_LIST_DIR}/libqt5xcbqpa-dev/${Qt6_VERSION})
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../xcb)
