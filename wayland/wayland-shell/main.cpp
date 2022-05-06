#include "../../src/vtablehook.h"

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

#include "dwaylandshellmanager.h"

// 用于窗口设置和dwayland相关的特殊属性的前缀
// 以_d_dwayland_开头的属性需要做特殊处理，一般是用于和kwayland的交互
#define _DWAYALND_ "_d_dwayland_"

DPP_USE_NAMESPACE

//using namespace std;

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

    VtableHook::overrideVfptrFun(shell, &QWaylandShellIntegration::createShellSurface, DWaylandShellManager::createShellSurface);

    KWayland::Client::Registry *registry = new KWayland::Client::Registry();
    wl_display *wlDisplay = reinterpret_cast<wl_display*>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration(QByteArrayLiteral("display")));
    registry->create(wlDisplay);
    Q_ASSERT_X(registry, "Registry", "KWayland create Registry failed.");

    connect(registry, &KWayland::Client::Registry::plasmaShellAnnounced,
            this, [registry] (quint32 name, quint32 version) {
        DWaylandShellManager::createKWaylandShell(registry, name, version);
    });
    connect(registry, &KWayland::Client::Registry::serverSideDecorationManagerAnnounced,
            this, [registry] (quint32 name, quint32 version) {
        DWaylandShellManager::createKWaylandSSD(registry, name, version);
    });

    //创建ddeshell
    connect(registry, &KWayland::Client::Registry::ddeShellAnnounced, [registry](quint32 name, quint32 version) {
       DWaylandShellManager::createDDEShell(registry, name, version);
    });

    //创建ddeseat
    connect(registry, &KWayland::Client::Registry::ddeSeatAnnounced, [registry](quint32 name, quint32 version) {
       DWaylandShellManager::createDDESeat(registry, name, version);
    });

    connect(registry, &KWayland::Client::Registry::interfacesAnnounced, [registry] {
       DWaylandShellManager::createDDEPointer(registry);
       DWaylandShellManager::createDDEKeyboard(registry);
       DWaylandShellManager::createDDEFakeInput(registry);
    });

    connect(registry, &KWayland::Client::Registry::strutAnnounced, [registry](quint32 name, quint32 version) {
       DWaylandShellManager::createStrut(registry, name, version);
    });

    registry->setup();

    wl_display_roundtrip(wlDisplay);

    return shell;
}

}

#include "main.moc"
