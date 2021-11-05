#-------------------------------------------------
#
# Project created by QtCreator 2020-04-29T13:23:01
#
#-------------------------------------------------

TARGET = dwayland
TEMPLATE = lib

CONFIG += link_pkgconfig plugin
PKGCONFIG += Qt5WaylandClient

# Qt >= 5.8
greaterThan(QT_MINOR_VERSION, 7): QT += gui-private
else: QT += platformsupport-private

DESTDIR = $$_PRO_FILE_PWD_/../../bin/plugins/platforms

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
        $$PWD/dwaylandinterfacehook.cpp \
        $$PWD/main.cpp \
        $$PWD/dwaylandintegration.cpp \
        $$PWD/dhighdpi.cpp

HEADERS += \
        $$PWD/dwaylandinterfacehook.h \
        $$PWD/dwaylandintegration.h \
        $$PWD/dhighdpi.h

qtHaveModule(waylandclient_private) : QT += waylandclient_private
else: INCLUDEPATH += $$PWD/../qtwayland-dev

include($$PWD/../../src/src.pri)

OTHER_FILES += \
    dwayland.json

isEmpty(INSTALL_PATH) {
    target.path = $$[QT_INSTALL_PLUGINS]/platforms
} else {
    target.path = $$INSTALL_PATH
}

INSTALLS += target
