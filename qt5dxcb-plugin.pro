#-------------------------------------------------
#
# Project created by QtCreator 2016-08-10T19:46:44
#
#-------------------------------------------------

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = DXcbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -

QT       += opengl x11extras
QT       += core-private gui-private platformsupport-private xcb_qpa_lib-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

TARGET = dxcb
TEMPLATE = lib
VERSION = $$QT_VERSION
CONFIG += plugin c++11

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/dxcbintegration.cpp \
    $$PWD/dxcbbackingstore.cpp \
    $$PWD/vtablehook.cpp \
    $$PWD/xcbwindowhook.cpp \
    $$PWD/utility.cpp

HEADERS += \
    $$PWD/dxcbintegration.h \
    $$PWD/dxcbbackingstore.h \
    $$PWD/vtablehook.h \
    $$PWD/xcbwindowhook.h \
    $$PWD/utility.h \
    global.h

DISTFILES += \
    $$PWD/dxcb.json

unix {
    target.path = $$[QT_INSTALL_PLUGINS]/platforms
    INSTALLS += target
}

INCLUDEPATH += $$PWD/libqt5xcbqpa-dev

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

message(Qt Version: $$VERSION)

exists($$PWD/libqt5xcbqpa-dev) {
    !system(cd $$PWD/libqt5xcbqpa-dev && git checkout $$VERSION) {
        !system(git fetch -p):error(update libqt5xcbqpa header sources failed)
        !system(git checkout $$VERSION)error(Not support Qt Version: $$VERSION)
    }
} else {
    !system(git clone https://github.com/zccrs/libqt5xcbqpa-dev.git):error(clone libqt5xcbqpa header sources failed)
    !system(cd $$PWD/libqt5xcbqpa-dev && git checkout $$VERSION):error(Not support Qt Version: $$VERSION)
}
