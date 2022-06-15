TEMPLATE = app

CONFIG += qt

QT += core
QT += gui
QT += core-private
QT += gui-private
QT += waylandclient
QT += waylandclient-private

CONFIG += c++17

SOURCES += main.cpp
TARGET = wayland_test
