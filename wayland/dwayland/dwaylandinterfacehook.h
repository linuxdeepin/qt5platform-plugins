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

#include "../../src/global.h"

#include <QtGlobal>
#include <qwindowdefs.h>

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
    static DWaylandInterfaceHook *instance() {
        static DWaylandInterfaceHook *hook = new DWaylandInterfaceHook;
        return hook;
    }

    static QFunctionPointer platformFunction(QPlatformNativeInterface *interface, const QByteArray &function);

    static bool setEnableNoTitlebar(QWindow *window, bool enable);
    static bool isEnableNoTitlebar(QWindow *window);
    static bool setWindowRadius(QWindow *window, int value);
    static void setWindowProperty(QWindow *window, const char *name, const QVariant &value);
    static void popupSystemWindowMenu(quintptr wid);
    static bool enableDwayland(QWindow *window);
    static bool isEnableDwayland(const QWindow *window);
    static void splitWindowOnScreen(WId wid, quint32 type);
    static bool supportForSplittingWindow(WId wid);

    void initXcb();
    static bool buildNativeSettings(QObject *object, quint32 settingWindow);
    static void clearNativeSettings(quint32 settingWindow);
    static DXcbXSettings *globalSettings();

private:
    DWaylandInterfaceHook() {}
    ~DWaylandInterfaceHook() {}

    QPlatformNativeInterface* m_nativeinterface = nullptr;
    xcb_connection_t *xcb_connection = nullptr;
    DXcbXSettings *m_xsettings = nullptr;
};

DPP_END_NAMESPACE

#endif // DPLATFORMNATIVEINTERFACE_H
