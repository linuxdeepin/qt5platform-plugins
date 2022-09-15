// SPDX-FileCopyrightText: 2017 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#define private public
#include "QtWaylandClient/private/qwaylandnativeinterface_p.h"
#undef private

#include "dwaylandinterfacehook.h"
#include "dhighdpi.h"
#include "dxsettings.h"
#include "dnotitlebarwindowhelper_wl.h"

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

bool DWaylandInterfaceHook::buildNativeSettings(QObject *object, quint32 settingWindow) {
    return dXSettings->buildNativeSettings(object, settingWindow);
}

void DWaylandInterfaceHook::clearNativeSettings(quint32 settingWindow) {
    dXSettings->clearNativeSettings(settingWindow);
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
    DNoTitlebarWlWindowHelper::requestByWindowProperty(window, ::supportForSplittingWindow);
    return window->property(::supportForSplittingWindow).toBool();
}

DPP_END_NAMESPACE
