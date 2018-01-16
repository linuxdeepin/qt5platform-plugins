#-------------------------------------------------
#
# Project created by QtCreator 2016-08-10T19:46:44
#
#-------------------------------------------------

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = DXcbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -

DESTDIR = $$_PRO_FILE_PWD_/../bin/plugins/platforms

QT       += opengl x11extras
QT       += core-private #xcb_qpa_lib-private
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets widgets-private
    # Qt >= 5.8
    greaterThan(QT_MINOR_VERSION, 7): QT += gui-private
    else: QT += platformsupport-private

    # Qt >= 5.10
    greaterThan(QT_MINOR_VERSION, 9): QT += edid_support-private
}

TEMPLATE = lib

isEmpty(VERSION) {
    isEmpty(VERSION): VERSION = $$system(git describe --tags --abbrev=0)
    isEmpty(VERSION): VERSION = $$DXCB_VERSION
    isEmpty(VERSION): error(VERSION is empty)
    VERSION = $$replace(VERSION, [^0-9.],)
}

DEFINES += DXCB_VERSION=\\\"$$VERSION\\\"

linux: include($$PWD/linux.pri)
windows: include($$PWD/windows.pri)

CONFIG += plugin c++11

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/dplatformintegration.cpp \
    $$PWD/vtablehook.cpp \
    $$PWD/dplatformnativeinterfacehook.cpp

HEADERS += \
    $$PWD/dplatformintegration.h \
    $$PWD/vtablehook.h \
    $$PWD/utility.h \
    $$PWD/global.h \
    $$PWD/dplatformnativeinterfacehook.h \
    $$PWD/dforeignplatformwindow.h \
    $$PWD/dwmsupport.h

DISTFILES += \
    $$PWD/dpp.json

isEmpty(INSTALL_PATH) {
    target.path = $$[QT_INSTALL_PLUGINS]/platforms
} else {
    target.path = $$INSTALL_PATH
}

message($$target.path)

INSTALLS += target

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT USE_NEW_IMPLEMENTING
} else {
    DEFINES += USE_NEW_IMPLEMENTING
}

contains(DEFINES, USE_NEW_IMPLEMENTING) {
    SOURCES += \
        $$PWD/dframewindow.cpp \
        $$PWD/dplatformwindowhelper.cpp \
        $$PWD/dplatformbackingstorehelper.cpp \
        $$PWD/dplatformopenglcontexthelper.cpp

    HEADERS += \
        $$PWD/dframewindow.h \
        $$PWD/dplatformwindowhelper.h \
        $$PWD/dplatformbackingstorehelper.h \
        $$PWD/dplatformopenglcontexthelper.h
} else {
    SOURCES += \
        $$PWD/dplatformbackingstore.cpp \
        $$PWD/dplatformwindowhook.cpp

    HEADERS += \
        $$PWD/dplatformbackingstore.h \
        $$PWD/dplatformwindowhook.h
}

RESOURCES += \
    cursors/cursor.qrc
