#ifndef DWaylandShellManager_H
#define DWaylandShellManager_H

#include "shell_common.h"

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
    static void createServerDecoration(QWaylandWindow *window);
    static void setGeometry(QPlatformWindow *self, const QRect &rect);
    static void pointerEvent(const QPointF &pointF, QEvent::Type type);
    static QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window);
    static void createKWaylandShell(quint32 name, quint32 version);
    static void createKWaylandSSD(quint32 name, quint32 version);
    static void createDDEShell(quint32 name, quint32 version);
    static void createDDESeat(quint32 name, quint32 version);
    static void createStrut(quint32 name, quint32 version);
    static void createBlur(quint32 name, quint32 version);
    static void createCompositor(quint32 name, quint32 version);
    static void createDDEPointer();
    static void createDDEKeyboard();
    static void createDDEFakeInput();
    static void handleGeometryChange(QWaylandWindow *window);
    static void handleWindowStateChanged(QWaylandWindow *window);
    static void setWindowStaysOnTop(QWaylandShellSurface *surface, const bool state);
    static void setDockStrut(QWaylandShellSurface *surface, const QVariant var);
    static void setCursorPoint(QPointF pos);

private:
    // 用于记录设置过以_DWAYALND_开头的属性，当kwyalnd_shell对象创建以后要使这些属性生效
    static QList<QPointer<QWaylandWindow>> send_property_window_list;
};
}

#endif // DWaylandShellManager_H
