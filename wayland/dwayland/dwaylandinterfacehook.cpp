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

#include "dnotitlebarwindowhelper_wl.h"

#include "dwaylandinterfacehook.h"
#include "dxcbxsettings.h"
#include "dnativesettings.h"
#include "dhighdpi.h"

#include <xcb/xcb.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <QtWaylandClientVersion>

#include <QDebug>
#include <QLoggingCategory>

#ifndef QT_DEBUG
Q_LOGGING_CATEGORY(dwli, "dtk.wayland.interface" , QtInfoMsg);
#else
Q_LOGGING_CATEGORY(dwli, "dtk.wayland.interface");
#endif

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
    } else if (function == setEnableNoTitlebar) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::setEnableNoTitlebar);
    } else if (function == isEnableNoTitlebar) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::isEnableNoTitlebar);
    } else if (function == setWindowRadius) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::setWindowRadius);
    } else if (function == setWindowProperty) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::setWindowProperty);
    } else if (function == popupSystemWindowMenu) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::popupSystemWindowMenu);
    } else if (function == enableDwayland) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::enableDwayland);
    } else if (function == isEnableDwayland) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::isEnableDwayland);
    } else if (function == splitWindowOnScreen) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::splitWindowOnScreen);
    } else if (function == supportForSplittingWindow) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::supportForSplittingWindow);
    } else if (function == enableBlurWindow) {
        return reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::enableBlurWindow);
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

#if QTWAYLANDCLIENT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    return static_cast<QtWaylandClient::QWaylandNativeInterface*>(interface)->QtWaylandClient::QWaylandNativeInterface::platformFunction(function);
#endif

    return nullptr;
}

void DWaylandInterfaceHook::init()
{
    static bool isInit = false;

    if (isInit && xcb_connection) {
        return;
    }

    isInit = true;
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

bool DWaylandInterfaceHook::setEnableNoTitlebar(QWindow *window, bool enable)
{
    if (enable) {
        if (DNoTitlebarWlWindowHelper::mapped.value(window))
            return true;
        if (window->type() == Qt::Desktop)
            return false;
        window->setProperty(noTitlebar, true);
        Q_UNUSED(new DNoTitlebarWlWindowHelper(window))
        return true;
    } else {
        if (auto helper = DNoTitlebarWlWindowHelper::mapped.value(window)) {
            helper->deleteLater();
        }
        window->setProperty(noTitlebar, false);
    }

    return true;
}

bool DWaylandInterfaceHook::isEnableNoTitlebar(QWindow *window)
{
    return window->property(noTitlebar).toBool();
}

bool DWaylandInterfaceHook::setWindowRadius(QWindow *window, int value)
{
    if (!window)
        return false;
    return window->setProperty(windowRadius, QVariant{value});
}

void DWaylandInterfaceHook::setWindowProperty(QWindow *window, const char *name, const QVariant &value)
{
    DNoTitlebarWlWindowHelper::setWindowProperty(window, name, value);
}

void DWaylandInterfaceHook::popupSystemWindowMenu(WId wid)
{
    DNoTitlebarWlWindowHelper::popupSystemWindowMenu(wid);
}

bool DWaylandInterfaceHook::enableDwayland(QWindow *window)
{
    static bool xwayland = QByteArrayLiteral("wayland") == qgetenv("XDG_SESSION_TYPE")
            && !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");

    if (xwayland) {
        // for xwayland
        return false;
    }

    if (window->type() == Qt::Desktop)
        return false;

    QPlatformWindow *xw = static_cast<QPlatformWindow*>(window->handle());

    if (!xw) {
        window->setProperty(useDwayland, true);

        return true;
    }
    if (DNoTitlebarWlWindowHelper::mapped.value(window))
        return true;

    if (xw->isExposed())
        return false;

#ifndef USE_NEW_IMPLEMENTING
    return false;
#endif

    window->setProperty(useDwayland, true);
    // window->setProperty("_d_dwayland_TransparentBackground", window->format().hasAlpha());

    return true;
}

bool DWaylandInterfaceHook::isEnableDwayland(const QWindow *window)
{
    return window->property(useDwayland).toBool();
}

void DWaylandInterfaceHook::splitWindowOnScreen(WId wid, quint32 type)
{
    QWindow *window = fromQtWinId(wid);
    if(!window || !window->handle())
        return;
    // 1 left,2 right,15 fullscreen
    if (type == 15) {
        if (window->windowStates().testFlag(Qt::WindowMaximized)) {
            window->showNormal();
        } else {
            window->showMaximized();
        }
    } else if (type == 1 || type == 2) {
        DNoTitlebarWlWindowHelper::setWindowProperty(window, ::splitWindowOnScreen, type);
    } else {
        qCWarning(dwli) << "invalid split type: " << type;
    }
}

bool DWaylandInterfaceHook::supportForSplittingWindow(WId wid)
{
    QWindow *window = fromQtWinId(wid);
    if(!window || !window->handle())
        return false;
    DNoTitlebarWlWindowHelper::setWindowProperty(window, ::supportForSplittingWindow, false);
    return window->property(::supportForSplittingWindow).toBool();
}

bool DWaylandInterfaceHook::enableBlurWindow(WId wid, bool enable)
{
    QWindow *window = fromQtWinId(wid);
    if(!window || !window->handle())
        return false;

    DNoTitlebarWlWindowHelper::setWindowProperty(window, ::enableBlurWindow, enable);
    return window->property(::enableBlurWindow).toBool();
}

DXcbXSettings *DWaylandInterfaceHook::globalSettings()
{
    if (Q_LIKELY(m_xsettings)) {
        return m_xsettings;
    }

    if (!xcb_connection) {
        init();
    }
    m_xsettings = new DXcbXSettings(xcb_connection);

    return m_xsettings;
}

DPP_END_NAMESPACE
