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

#include "dwaylandinterfacehook.h"
#include "dxcbxsettings.h"
#include "dnativesettings.h"

#include <xcb/xcb.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

DPP_BEGIN_NAMESPACE


class DXcbEventFilter : public QThread
{
public:
    DXcbEventFilter(xcb_connection_t *connection)
        : m_connection(connection)
    {
        QThread::start();
    }

    void run() override {
        xcb_generic_event_t *event;
        while (m_connection && (event = xcb_wait_for_event(m_connection))) {
            uint response_type = event->response_type & ~0x80;
            switch (response_type) {
                case XCB_PROPERTY_NOTIFY: {
                    xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)event;
                    DXcbXSettings::handlePropertyNotifyEvent(pn);
                    break;
                }

                case XCB_CLIENT_MESSAGE: {
                    xcb_client_message_event_t *ev = reinterpret_cast<xcb_client_message_event_t*>(event);
                    DXcbXSettings::handleClientMessageEvent(ev);
                    break;
                }
            }
        }
    }

private:
    xcb_connection_t *m_connection;
};

static QFunctionPointer getFunction(const QByteArray &function)
{
    if (function == buildNativeSettings) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::buildNativeSettings);
    } else if (function == clearNativeSettings) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::clearNativeSettings);
    }

    return nullptr;
}

thread_local QHash<QByteArray, QFunctionPointer> DWaylandInterfaceHook::functionCache;
xcb_connection_t *DWaylandInterfaceHook::xcb_connection = nullptr;
DXcbXSettings *DWaylandInterfaceHook::m_xsettings = nullptr;
QFunctionPointer DWaylandInterfaceHook::platformFunction(QPlatformNativeInterface *interface, const QByteArray &function)
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

void DWaylandInterfaceHook::init()
{
    int primary_screen_number = 0;
    xcb_connection = xcb_connect(qgetenv("DISPLAY"), &primary_screen_number);

    new DXcbEventFilter(xcb_connection);
}

bool DWaylandInterfaceHook::buildNativeSettings(QObject *object, quint32 settingWindow)
{
    QByteArray settings_property = DNativeSettings::getSettingsProperty(object);
    DXcbXSettings *settings = nullptr;
    bool global_settings = false;
    if (settingWindow || !settings_property.isEmpty()) {
        settings = new DXcbXSettings(xcb_connection, settingWindow, settings_property);
    } else {
        global_settings = true;
        settings = globalSettings();
    }

    // 跟随object销毁
    auto native_settings = new DNativeSettings(object, settings, global_settings);

    if (!native_settings->isValid()) {
        delete native_settings;
        return false;
    }

    return true;
}

void DWaylandInterfaceHook::clearNativeSettings(quint32 settingWindow)
{
#ifdef Q_OS_LINUX
    DXcbXSettings::clearSettings(settingWindow);
#endif
}

DXcbXSettings *DWaylandInterfaceHook::globalSettings()
{
    if (Q_LIKELY(m_xsettings))
        return m_xsettings;

    m_xsettings = new DXcbXSettings(xcb_connection);
    return m_xsettings;
}

DPP_END_NAMESPACE
