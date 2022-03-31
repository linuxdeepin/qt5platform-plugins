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
#undef private

#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/server_decoration.h>
#include <KWayland/Client/ddeshell.h>
#include <KWayland/Client/ddeseat.h>
#include <KWayland/Client/ddekeyboard.h>
#include <KWayland/Client/strut.h>
#include <KWayland/Client/fakeinput.h>

#include <QGuiApplication>
#include <QPointer>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <private/qwidgetwindow_p.h>

DPP_USE_NAMESPACE

namespace QtWaylandClient {

class DWaylandShellManager
{
public:
    DWaylandShellManager();
    ~DWaylandShellManager();
    static void sendProperty(QWaylandShellSurface *self, const QString &name, const QVariant &value);
    static void requestActivateWindow(QPlatformWindow *self);
    static bool disableClientDecorations(QWaylandShellSurface *surface);
    static QMargins frameMargins(QPlatformWindow *self);
    static void createServerDecoration(QWaylandWindow *window);
    static void setGeometry(QPlatformWindow *self, const QRect &rect);
    static void pointerEvent(const QPointF &pointF, QEvent::Type type);
    static void handleKeyEvent(quint32 key, KWayland::Client::DDEKeyboard::KeyState state, quint32 time);
    static void handleModifiersChanged(quint32 depressed, quint32 latched, quint32 locked, quint32 group);
    static QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window);
    static void createKWaylandShell(KWayland::Client::Registry *registry, quint32 name, quint32 version);
    static void createKWaylandSSD(KWayland::Client::Registry *registry, quint32 name, quint32 version);
    static void createDDEShell(KWayland::Client::Registry *registry, quint32 name, quint32 version);
    static void createDDESeat(KWayland::Client::Registry *registry, quint32 name, quint32 version);
    static void createStrut(KWayland::Client::Registry *registry, quint32 name, quint32 version);
    static void createDDEPointer(KWayland::Client::Registry *registry);
    static void createDDEKeyboard(KWayland::Client::Registry *registry);
    static void createDDEFakeInput(KWayland::Client::Registry *registry);
    static void handleGeometryChange(QWaylandWindow *window);
    static void handleWindowStateChanged(QWaylandWindow *window);
    static void setWindowStaysOnTop(QWaylandShellSurface *surface, const bool state);
    static void setDockStrut(QWaylandShellSurface *surface, const QVariant var);
    static void setCursorPoint(QPointF pos);
};

}

#endif // DWaylandShellManager_H
