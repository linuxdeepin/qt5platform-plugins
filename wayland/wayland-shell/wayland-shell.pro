#-------------------------------------------------
#
# Project created by QtCreator 2020-04-29T13:23:01
#
#-------------------------------------------------

QT       += KWaylandClient
LIBS     += -lKF5WaylandClient
CONFIG += link_pkgconfig plugin
PKGCONFIG += Qt5WaylandClient

# Qt >= 5.8
greaterThan(QT_MINOR_VERSION, 7): QT += gui-private widgets-private
else: QT += platformsupport-private

TARGET = kwayland-shell
TEMPLATE = lib

load(configure)
qtCompileTest(deepin-kwin-test) {
    DEFINES += D_DEEPIN_KWIN
}

DESTDIR = $$_PRO_FILE_PWD_/../../bin/plugins/wayland-shell-integration
DEFINES += QT5DWAYLANDPLUGIN_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

isEmpty(VERSION) {
    isEmpty(VERSION): VERSION = $$system(git describe --tags --abbrev=0)
    VERSION = $$replace(VERSION, [^0-9.],)
    isEmpty(VERSION): VERSION = 1.1.11
}

SOURCES += \
        $$PWD/qt5dwaylandplugin.cpp \
        $$PWD/main.cpp \
    dwaylandshellmanager.cpp

HEADERS += \
        $$PWD/qt5dwaylandplugin.h \
        $$PWD/qt5dwayland-plugin_global.h \
    dwaylandshellmanager.h

qtHaveModule(waylandclient_private) : QT += waylandclient_private
else: INCLUDEPATH += $$PWD/../qtwayland-dev

include($$PWD/../../src/src.pri)

OTHER_FILES += \
    kwayland-shell.json

isEmpty(INSTALL_PATH) {
    target.path = $$[QT_INSTALL_PLUGINS]/wayland-shell-integration
} else {
    target.path = $$INSTALL_PATH
}

INSTALLS += target
