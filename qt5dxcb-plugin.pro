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
    $$PWD/genericplugin.cpp \
    $$PWD/dxcbbackingstore.cpp \
    $$PWD/vtablehook.cpp \
    $$PWD/xcbwindowhook.cpp \
    $$PWD/utility.cpp

HEADERS += \
    $$PWD/genericplugin.h \
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

INCLUDEPATH += $$PWD/xcb

#DEFINES += QT_NO_DEBUG_OUTPUT
