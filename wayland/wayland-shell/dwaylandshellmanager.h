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

struct BlurArea {
    qint32 x;
    qint32 y;
    qint32 width;
    qint32 height;
    qint32 xRadius;
    qint32 yRaduis;

    inline BlurArea operator *(qreal scale)
    {
        if (qFuzzyCompare(1.0, scale))
            return *this;

        BlurArea new_area;

        new_area.x = qRound64(x * scale);
        new_area.y = qRound64(y * scale);
        new_area.width = qRound64(width * scale);
        new_area.height = qRound64(height * scale);
        new_area.xRadius = qRound64(xRadius * scale);
        new_area.yRaduis = qRound64(yRaduis * scale);

        return new_area;
    }

    inline BlurArea &operator *=(qreal scale)
    {
        return *this = *this * scale;
    }
};

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
    static void setEnableBlurWidow(QWaylandWindow *wlWindow);
    static void updateWindowBlurAreasForWM(QWaylandWindow *wlWindow, const QString &name, const QVariant &value);

private:
    // 用于记录设置过以_DWAYALND_开头的属性，当kwyalnd_shell对象创建以后要使这些属性生效
    static QList<QPointer<QWaylandWindow>> send_property_window_list;
    static bool m_enableBlurWidow;
};
}

#endif // DWaylandShellManager_H
