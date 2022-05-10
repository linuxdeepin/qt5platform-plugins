#include "vtablehook.h"
#include "dwaylandshellmanager.h"

#define private public
#include "QtWaylandClient/private/qwaylandintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellintegrationplugin_p.h"
#include "QtWaylandClient/private/qwaylandshellintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandwindow_p.h"
#undef private

#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/server_decoration.h>
#include <KWayland/Client/ddeshell.h>

#include <QGuiApplication>
#include <QPointer>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <private/qwidgetwindow_p.h>

// 用于窗口设置和dwayland相关的特殊属性的前缀
// 以_d_dwayland_开头的属性需要做特殊处理，一般是用于和kwayland的交互
#define _DWAYALND_ "_d_dwayland_"

DPP_USE_NAMESPACE

namespace QtWaylandClient {

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

    HookOverride(shell, &QWaylandShellIntegration::createShellSurface, DWaylandShellManager::createShellSurface);

    Registry *registry = DWaylandShellManager::registry();
    wl_display *wlDisplay = reinterpret_cast<wl_display*>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration(QByteArrayLiteral("display")));
    registry->create(wlDisplay);

    connect(registry, &Registry::plasmaShellAnnounced,
            &DWaylandShellManager::createKWaylandShell);

    connect(registry, &Registry::serverSideDecorationManagerAnnounced,
            &DWaylandShellManager::createKWaylandSSD);

    //创建ddeshell
    connect(registry, &Registry::ddeShellAnnounced,
            &DWaylandShellManager::createDDEShell);

    //创建ddeseat
    connect(registry, &KWayland::Client::Registry::ddeSeatAnnounced,
            &DWaylandShellManager::createDDESeat);

    connect(registry, &KWayland::Client::Registry::interfacesAnnounced, [] {
        DWaylandShellManager::createDDEPointer();
        DWaylandShellManager::createDDEKeyboard();
        DWaylandShellManager::createDDEFakeInput();
    });

    connect(registry, &KWayland::Client::Registry::strutAnnounced,
            &DWaylandShellManager::createStrut);

    registry->setup();

    wl_display_roundtrip(wlDisplay);

    return shell;
}

}

#include "main.moc"
