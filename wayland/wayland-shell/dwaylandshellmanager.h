// SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DWaylandShellManager_H
#define DWaylandShellManager_H

#include "vtablehook.h"

#define private public
#include "QtWaylandClient/private/qwaylandintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellintegrationplugin_p.h"
#include "QtWaylandClient/private/qwaylandshellintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandwindow_p.h"
#include "QtWaylandClient/private/qwaylandcursor_p.h"
#include "QtWaylandClient/private/qwaylandscreen_p.h"
#undef private

#include <DWayland/Client/registry.h>
#include <DWayland/Client/plasmashell.h>
#include <DWayland/Client/server_decoration.h>
#include <DWayland/Client/ddeshell.h>
#include <DWayland/Client/ddeseat.h>
#include <DWayland/Client/ddekeyboard.h>
#include <DWayland/Client/strut.h>
#include <DWayland/Client/fakeinput.h>
#include <DWayland/Client/compositor.h>
#include <DWayland/Client/blur.h>
#include <DWayland/Client/region.h>
#include <DWayland/Client/surface.h>

#include <QGuiApplication>
#include <QPointer>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <private/qwidgetwindow_p.h>

DPP_USE_NAMESPACE
using namespace KWayland::Client;

namespace QtWaylandClient {

class DWaylandShellManager
{
    DWaylandShellManager();
    ~DWaylandShellManager();
    Registry *m_registry = nullptr;
public:
    static DWaylandShellManager *instance() {
        static DWaylandShellManager manager;
        return &manager;
    }
    static Registry *registry() {
        return instance()->m_registry;
    }

    static void sendProperty(QWaylandShellSurface *self, const QString &name, const QVariant &value);
    static void requestActivateWindow(QPlatformWindow *self);
    static bool disableClientDecorations(QWaylandShellSurface *surface);
    static QMargins frameMargins(QPlatformWindow *self);
    static void setWindowFlags(QPlatformWindow *self, Qt::WindowFlags flags);
    static void createServerDecoration(QWaylandWindow *window);
    static void setGeometry(QPlatformWindow *self, const QRect &rect);
    static void pointerEvent(const QPointF &pointF, QEvent::Type type);
    static QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window);
    static void createKWaylandShell(quint32 name, quint32 version);
    static void createKWaylandSSD(quint32 name, quint32 version);
    static void createDDEShell(quint32 name, quint32 version);
    static void createDDESeat(quint32 name, quint32 version);
    static void createStrut(quint32 name, quint32 version);
    static void createDDEPointer();
    static void createDDEKeyboard();
    static void createDDEFakeInput();
    static void createBlurManager(quint32 name, quint32 version);
    static void createCompositor(quint32 name, quint32 version);
    static void createSurface();
    static void handleGeometryChange(QWaylandWindow *window);
    static void handleWindowStateChanged(QWaylandWindow *window);
    static void setWindowStaysOnTop(QWaylandShellSurface *surface, const bool state);
    static void setDockStrut(QWaylandShellSurface *surface, const QVariant var);
    static void setCursorPoint(QPointF pos);
    static void setEnableBlurWidow(QWaylandWindow *wlWindow, const QVariant &value);
    static void updateWindowBlurAreasForWM(QWaylandWindow *wlWindow, const QString &name, const QVariant &value);

private:
    // 用于记录设置过以_DWAYALND_开头的属性，当kwyalnd_shell对象创建以后要使这些属性生效
    static QList<QPointer<QWaylandWindow>> send_property_window_list;
};
}

#endif // DWaylandShellManager_H
