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
#define private public
#include "QtWaylandClient/private/qwaylandnativeinterface_p.h"
#undef private

#include "dplatformnativeinterfacehook.h"
#include "dxcbxsettings.h"
#include "dnativesettings.h"
#include "waylandnativeeventfilter.h"

#include <qxcbconnection.h>
#include <qpa/qplatformnativeinterface.h>
#include <QInputEvent>
#include <QStringLiteral>
#include <QCoreApplication>
#include <QString>
#include <QHash>
#include <QtGlobal>

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
xcb_connection_t *DPlatformNativeInterfaceHook::xcb_connection = nullptr;
DXcbXSettings *DPlatformNativeInterfaceHook::m_xsettings = nullptr;
QFunctionPointer DPlatformNativeInterfaceHook::platformFunction(QPlatformNativeInterface *interface, const QByteArray &function)
{
    if (QFunctionPointer f = functionCache.value(function)) {
        return f;
    }

    QFunctionPointer f = getFunction(function);

    if (f) {
        functionCache.insert(function, f);
        return f;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    return static_cast<QtWaylandClient::QWaylandNativeInterface*>(interface)->QtWaylandClient::QWaylandNativeInterface::platformFunction(function);
#endif

    return nullptr;
}

void DPlatformNativeInterfaceHook::init()
{
    bool can_grab = true;
    static bool can_not_grab_env = qEnvironmentVariableIsSet("QT_XCB_NO_GRAB_SERVER");
    if(can_not_grab_env) can_grab = false;

    int primary_screen_number = 0;
    xcb_connection = xcb_connect(qgetenv("DISPLAY").isEmpty() ? ":0" : qgetenv("DISPLAY"), &primary_screen_number);

//    auto m_eventfilter = new WaylandNativeEventFilter(xcb_connection);
//    qApp->installNativeEventFilter(m_eventfilter);
}

bool DPlatformNativeInterfaceHook::buildNativeSettings(QObject *object, quint32 settingWindow)
{
    QByteArray settings_property = DNativeSettings::getSettingsProperty(object);
    DXcbXSettings *settings = nullptr;
    bool global_settings = false;
    if (settingWindow || !settings_property.isEmpty()) {
        settings = new DXcbXSettings(xcb_connection, settingWindow, settings_property);
    } else {
        global_settings = true;
        settings = !m_xsettings ? new DXcbXSettings(xcb_connection) : m_xsettings;
    }

    // 跟随object销毁
    auto native_settings = new DNativeSettings(object, settings, global_settings);

    if (!native_settings->isValid()) {
        delete native_settings;
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
