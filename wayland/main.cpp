#define private public
#include "QtWaylandClient/private/qwaylandintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellintegrationplugin_p.h"
#include "QtWaylandClient/private/qwaylandshellintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandwindow_p.h"
#undef private

#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#include "vtablehook.h"
#include "dplatformnativeinterfacehook.h"
#include <private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <QGuiApplication>
#include <QPointer>
#include <QDebug>

#define WINDOWS_DOCK 0x00000081

DPP_USE_NAMESPACE

namespace QtWaylandClient {
static QPointer<KWayland::Client::PlasmaShell> kwayland_shell;
static QList<QPointer<QWaylandWindow>> lw_window_list;
static QList<QPointer<QWaylandWindow>> fg_window_list;

static KWayland::Client::PlasmaShellSurface* createKWayland(QWaylandWindow *window)
{
    if (!window)
        return nullptr;

    if (kwayland_shell) {
        auto surface = window->shellSurface();
        return kwayland_shell->createSurface(window->object(), surface);
    }

    return nullptr;
}

static void sendProperty(QWaylandShellSurface *self, const QString &name, const QVariant &value)
{
    auto *ksurface = self->findChild<KWayland::Client::PlasmaShellSurface*>();

    if (!ksurface) {
        ksurface = createKWayland(self->window());
    }

    if (QStringLiteral("window-position") == name) {
        if (ksurface) {
            ksurface->setPosition(value.toPoint());
        } else {
            lw_window_list << self->window();
        }

        return;
    } else if((QStringLiteral("window-flags") == name) && ( (value.toULongLong() & WINDOWS_DOCK) == WINDOWS_DOCK)) {
        if (ksurface) {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
            ksurface->setPanelBehavior(KWayland::Client::PlasmaShellSurface::PanelBehavior::AlwaysVisible);
        } else {
            fg_window_list << self->window();
        }

        return;
    }

    VtableHook::callOriginalFun(self, &QWaylandShellSurface::sendProperty, name, value);
}

static void setGeometry(QPlatformWindow *self, const QRect &rect)
{
    VtableHook::callOriginalFun(self, &QPlatformWindow::setGeometry, rect);

    if (!self->QPlatformWindow::parent()) {
        if (auto lw_window = static_cast<QWaylandWindow*>(self)) {
            lw_window->sendProperty(QStringLiteral("window-position"), rect.topLeft());
        }
    }
}

static void setWindowFlags(QWaylandWindow *self, Qt::WindowFlags flags)
{
    VtableHook::callOriginalFun(self, &QPlatformWindow::setWindowFlags, flags);

    if (!self->QPlatformWindow::parent()) {
        if (auto fg_window = static_cast<QWaylandWindow*>(self)) {
            fg_window->sendProperty(QStringLiteral("window-flags"), (qulonglong)flags);
        }
    }
}

static QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window)
{
    auto surface = VtableHook::callOriginalFun(self, &QWaylandShellIntegration::createShellSurface, window);

    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::sendProperty, sendProperty);
    VtableHook::overrideVfptrFun(window, &QPlatformWindow::setGeometry, setGeometry);
    VtableHook::overrideVfptrFun(window, &QWaylandWindow::setWindowFlags, setWindowFlags);

    return surface;
}

class QKWaylandShellIntegrationPlugin : public QWaylandShellIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QWaylandShellIntegrationFactoryInterface_iid FILE "kwayland-shell.json")

public:
    QWaylandShellIntegration *create(const QString &key, const QStringList &paramList) override;
};

QWaylandShellIntegration *QKWaylandShellIntegrationPlugin::create(const QString &key, const QStringList &paramList)
{
    Q_UNUSED(key)
    Q_UNUSED(paramList)
    auto wayland_integration = static_cast<QWaylandIntegration *>(QGuiApplicationPrivate::platformIntegration());
    auto shell = wayland_integration->createShellIntegration("xdg-shell-v6");

    VtableHook::overrideVfptrFun(wayland_integration->nativeInterface(), &QPlatformNativeInterface::platformFunction, &DPlatformNativeInterfaceHook::platformFunction);
    VtableHook::overrideVfptrFun(shell, &QWaylandShellIntegration::createShellSurface, createShellSurface);

    KWayland::Client::Registry *registry = new KWayland::Client::Registry();
    registry->create(reinterpret_cast<wl_display*>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration(QByteArrayLiteral("display"))));
    connect(registry, &KWayland::Client::Registry::plasmaShellAnnounced,
            this, [registry] (quint32 name, quint32 version) {
        kwayland_shell = registry->createPlasmaShell(name, version, registry->parent());

        qDebug() << lw_window_list;

        for (QPointer<QWaylandWindow> lw_window : lw_window_list) {
            if (lw_window) {
                sendProperty(lw_window->shellSurface(), "window-position", lw_window->geometry().topLeft());
            }
        }

        for (QPointer<QWaylandWindow> fg_window : fg_window_list) {
            if (fg_window) {
                sendProperty(fg_window->shellSurface(), "window-flags", (qulonglong)fg_window->window()->flags());
            }
        }

        lw_window_list.clear();
        fg_window_list.clear();
    });
    registry->setup();

    return shell;
}

}

#include "main.moc"
