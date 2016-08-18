#-------------------------------------------------
#
# Project created by QtCreator 2016-08-10T19:46:44
#
#-------------------------------------------------

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = DXcbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -

Qt       += opengl
QT       += core-private gui-private platformsupport-private
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

TARGET = dxcb
TEMPLATE = lib
VERSION = 5.5.1
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
    $$PWD/utility.h

DISTFILES += \
    $$PWD/dxcb.json

unix {
    target.path = $$[QT_INSTALL_PLUGINS]/platforms
    INSTALLS += target
}

include($$PWD/xcb/libqt5xcbqpa.pri)

INCLUDEPATH += $$PWD/xcb

#DEFINES += QT_NO_DEBUG_OUTPUT
