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

#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/server_decoration.h>
#include <KWayland/Client/ddeshell.h>
#include <KWayland/Client/ddeseat.h>
#include <KWayland/Client/ddekeyboard.h>
#include <KWayland/Client/strut.h>
#include <KWayland/Client/fakeinput.h>
#include <KWayland/Client/compositor.h>
#include <KWayland/Client/blur.h>
#include <KWayland/Client/region.h>
#include <KWayland/Client/surface.h>

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
