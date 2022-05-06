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
#include "../../src/dxcbxsettings.h"
#include "../../src/dnativesettings.h"
#include "dhighdpi.h"

#include <xcb/xcb.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <QtWaylandClientVersion>
#include <QtGlobal>
#include <QFunctionPointer>

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
    ~DXcbEventFilter() override {
        if (QThread::isRunning())
            QThread::exit();
    }

    void run() override {
        xcb_generic_event_t *event;
        while (!xcb_connection_has_error(m_connection) && (event = xcb_wait_for_event(m_connection))) {
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
        delete this;
    }

private:
    xcb_connection_t *m_connection;
};

static QFunctionPointer getFunction(const QByteArray &function)
{
    static QHash<QByteArray, QFunctionPointer> functionCache = {
        {buildNativeSettings, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::buildNativeSettings)},
        {clearNativeSettings, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::clearNativeSettings)},
        {setEnableNoTitlebar, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::setEnableNoTitlebar)},
        {isEnableNoTitlebar, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::isEnableNoTitlebar)},
        {setWindowRadius, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::setWindowRadius)},
        {setWindowProperty, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::setWindowProperty)},
        {popupSystemWindowMenu, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::popupSystemWindowMenu)},
        {enableDwayland, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::enableDwayland)},
        {isEnableDwayland, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::isEnableDwayland)},
        {splitWindowOnScreen, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::splitWindowOnScreen)},
        {supportForSplittingWindow, reinterpret_cast<QFunctionPointer>(&DWaylandInterfaceHook::supportForSplittingWindow)}
    };
    return functionCache.value(function);
}

QFunctionPointer DWaylandInterfaceHook::platformFunction(QPlatformNativeInterface *interface, const QByteArray &function)
{
    QFunctionPointer f = getFunction(function);

#if QTWAYLANDCLIENT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    if (Q_UNLIKELY(!f)) {
        f = static_cast<QtWaylandClient::QWaylandNativeInterface*>(interface)->QtWaylandClient::QWaylandNativeInterface::platformFunction(function);
    }
#endif
    return f;
}

bool DWaylandInterfaceHook::setEnableNoTitlebar(QWindow *window, bool enable)
{
    auto helper = DNoTitlebarWlWindowHelper::mapped.value(window);
    if (Q_LIKELY(enable)) {
        if (Q_UNLIKELY(helper))
            return true;
        if (Q_UNLIKELY(window->type() == Qt::Desktop))
            return false;
        window->setProperty(noTitlebar, true);
        Q_UNUSED(new DNoTitlebarWlWindowHelper(window))
        return true;
    } else {
        if (Q_LIKELY(helper)) {
            helper->deleteLater();
        }
        window->setProperty(noTitlebar, false);
    }

    return true;
}

bool DWaylandInterfaceHook::isEnableNoTitlebar(QWindow *window)
{
    if (Q_UNLIKELY(!window))
        return false;
    return window->property(noTitlebar).toBool();
}

bool DWaylandInterfaceHook::setWindowRadius(QWindow *window, int value)
{
    if (Q_UNLIKELY(!window))
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
    if (Q_UNLIKELY(!window))
        return false;
    return window->property(useDwayland).toBool();
}

void DWaylandInterfaceHook::splitWindowOnScreen(WId wid, quint32 type)
{
    QWindow *window = fromQtWinId(wid);
    if(Q_UNLIKELY(!window || !window->handle()))
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
    if (Q_UNLIKELY(!window || !window->handle()))
        return false;
    DNoTitlebarWlWindowHelper::setWindowProperty(window, ::supportForSplittingWindow, false);
    return window->property(::supportForSplittingWindow).toBool();
}

void DWaylandInterfaceHook::initXcb()
{
    if (Q_UNLIKELY(xcb_connection)) {
        return;
    }
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
        settings = new DXcbXSettings(instance()->xcb_connection, settingWindow, settings_property);
    } else {
        global_settings = true;
        settings = instance()->globalSettings();
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
    if (Q_LIKELY(instance()->m_xsettings)) {
        return instance()->m_xsettings;
    }

    if (!instance()->xcb_connection) {
        instance()->initXcb();
    }
    instance()->m_xsettings = new DXcbXSettings(instance()->xcb_connection);

    return instance()->m_xsettings;
}

DPP_END_NAMESPACE
