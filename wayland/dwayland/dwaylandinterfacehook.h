/*
 * Copyright (C) 2017 ~ 2020 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DPLATFORMNATIVEINTERFACE_H
#define DPLATFORMNATIVEINTERFACE_H

#include "global.h"

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QObject;
class QWindow;
class QPointF;
class QVariant;
class QPlatformNativeInterface;
QT_END_NAMESPACE

typedef struct xcb_connection_t xcb_connection_t;

DPP_BEGIN_NAMESPACE

class DXcbXSettings;
class DWaylandInterfaceHook
{
public:
    static void init();
    static QFunctionPointer platformFunction(QPlatformNativeInterface *interface, const QByteArray &function);
    static thread_local QHash<QByteArray, QFunctionPointer> functionCache;

    static bool buildNativeSettings(QObject *object, quint32 settingWindow);
    static void clearNativeSettings(quint32 settingWindow);
    static bool setEnableNoTitlebar(QWindow *window, bool enable);
    static bool isEnableNoTitlebar(QWindow *window);
    static bool setWindowRadius(QWindow *window, int windowRadius);
    static void setWindowProperty(QWindow *window, const char *name, const QVariant &value);
    static void popupSystemWindowMenu(quintptr wid);
    static bool enableDwayland(QWindow *window);
    static bool isEnableDwayland(const QWindow *window);
    static DXcbXSettings *globalSettings();

private:
    static QPlatformNativeInterface* m_nativeinterface;
    static xcb_connection_t *xcb_connection;
    static DXcbXSettings *m_xsettings;
};

DPP_END_NAMESPACE

#endif // DPLATFORMNATIVEINTERFACE_H
