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

#include "dplatformnativeinterfacehook.h"
#include "global.h"
#include "qxcbconnection.h"
#include "dxcbxsettings.h"
#include "xcbnativeeventfilter.h"
#include "dnativesettings.h"

#include <QInputEvent>
#include <QStringLiteral>
#include <QCoreApplication>
#include <QString>
#include <QHash>
#include <QtGlobal>

#ifdef Q_OS_LINUX
#elif defined(Q_OS_WIN)
#include "qwindowsgdinativeinterface.h"
typedef QWindowsGdiNativeInterface DPlatformNativeInterface;
#endif

DPP_BEGIN_NAMESPACE

static QFunctionPointer getFunction(const QByteArray &function)
{
    if (function == buildNativeSettings) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformNativeInterfaceHook::buildNativeSettings);
    } else if (function == clearNativeSettings) {
        return reinterpret_cast<QFunctionPointer>(&DPlatformNativeInterfaceHook::clearNativeSettings);
    }

    return nullptr;
}

thread_local QHash<QByteArray, QFunctionPointer> DPlatformNativeInterfaceHook::functionCache;
QXcbConnection* DPlatformNativeInterfaceHook::xcb_connection;
QFunctionPointer DPlatformNativeInterfaceHook::platformFunction(QPlatformNativeInterface *interface, const QByteArray &function)
{
    Q_UNUSED(interface);
    if (QFunctionPointer f = functionCache.value(function)) {
        return f;
    }

    QFunctionPointer f = getFunction(function);

    if (f) {
        functionCache.insert(function, f);
        return f;
    }

    return nullptr;
}

void DPlatformNativeInterfaceHook::init(QPlatformNativeInterface *interface)
{
    bool can_grab = true;
    static bool can_not_grab_env = qEnvironmentVariableIsSet("QT_XCB_NO_GRAB_SERVER");
    if(can_not_grab_env) {
        can_grab = false;
    }

    xcb_connection = new QXcbConnection((QXcbNativeInterface*)interface, can_grab, UINT_MAX, nullptr);

    auto m_eventFilter = new XcbNativeEventFilter(xcb_connection);
    QCoreApplication::instance()->installNativeEventFilter(m_eventFilter);
}

bool DPlatformNativeInterfaceHook::buildNativeSettings(QObject *object, quint32 settingWindow)
{
    auto settings = new DNativeSettings(object, settingWindow, xcb_connection);

    if (!settings->isValid()) {
        delete settings;
        return false;
    }

    return true;
}

void DPlatformNativeInterfaceHook::clearNativeSettings(quint32 settingWindow)
{
#ifdef Q_OS_LINUX
    DXcbXSettings::clearSettings(settingWindow);
#endif
}

DPP_END_NAMESPACE
