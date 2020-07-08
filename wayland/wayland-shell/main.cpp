#include "vtablehook.h"

#define private public
#include "QtWaylandClient/private/qwaylandintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellintegrationplugin_p.h"
#include "QtWaylandClient/private/qwaylandshellintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandwindow_p.h"
#undef private

#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#include <QGuiApplication>
#include <QPointer>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

// 用于窗口设置和dwayland相关的特殊属性的前缀
// 以_d_dwayland_开头的属性需要做特殊处理，一般是用于和kwayland的交互
#define _DWAYALND_ "_d_dwayland_"

DPP_USE_NAMESPACE

namespace QtWaylandClient {
// kwayland中PlasmaShell的全局对象，用于使用kwayland中的扩展协议
static QPointer<KWayland::Client::PlasmaShell> kwayland_shell;
// 用于记录设置过以_DWAYALND_开头的属性，当kwyalnd_shell对象创建以后要使这些属性生效
static QList<QPointer<QWaylandWindow>> send_property_window_list;

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
    if (!name.startsWith(QStringLiteral(_DWAYALND_)))
        return VtableHook::callOriginalFun(self, &QWaylandShellSurface::sendProperty, name, value);

    auto *ksurface = self->findChild<KWayland::Client::PlasmaShellSurface*>();

    if (!ksurface) {
        ksurface = createKWayland(self->window());

        // 如果创建失败则说明kwaylnd_shell对象还未初始化，应当终止设置
        // 记录下本次的设置行为，kwayland_shell创建后会重新设置这些属性
        if (!ksurface) {
            send_property_window_list << self->window();
            return;
        }
    }

    if (QStringLiteral(_DWAYALND_ "window-position") == name) {
        ksurface->setPosition(value.toPoint());
    } else if (QStringLiteral(_DWAYALND_ "window-type") == name) {
        const QByteArray &type = value.toByteArray();

        if (type == "dock") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
            ksurface->setPanelBehavior(KWayland::Client::PlasmaShellSurface::PanelBehavior::AlwaysVisible);
        }
    }
}

static void setGeometry(QPlatformWindow *self, const QRect &rect)
{
    VtableHook::callOriginalFun(self, &QPlatformWindow::setGeometry, rect);

    if (!self->QPlatformWindow::parent()) {
        if (auto lw_window = static_cast<QWaylandWindow*>(self)) {
            lw_window->sendProperty(QStringLiteral(_DWAYALND_ "window-position"), rect.topLeft());
        }
    }
}

static QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window)
{
    auto surface = VtableHook::callOriginalFun(self, &QWaylandShellIntegration::createShellSurface, window);

    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::sendProperty, sendProperty);
    VtableHook::overrideVfptrFun(window, &QPlatformWindow::setGeometry, setGeometry);

    // 初始化设置窗口位置
    window->sendProperty(_DWAYALND_ "window-position", window->window()->geometry().topLeft());

    if (window->window()) {
        for (const QByteArray &pname : window->window()->dynamicPropertyNames()) {
            if (Q_LIKELY(!pname.startsWith(QByteArrayLiteral(_DWAYALND_))))
                continue;
            // 将窗口自定义属性记录到wayland window property中
            window->sendProperty(pname, window->window()->property(pname.constData()));
        }
    }

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

    VtableHook::overrideVfptrFun(shell, &QWaylandShellIntegration::createShellSurface, createShellSurface);

    KWayland::Client::Registry *registry = new KWayland::Client::Registry();
    registry->create(reinterpret_cast<wl_display*>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration(QByteArrayLiteral("display"))));

    connect(registry, &KWayland::Client::Registry::plasmaShellAnnounced,
            this, [registry] (quint32 name, quint32 version) {
        kwayland_shell = registry->createPlasmaShell(name, version, registry->parent());

        for (QPointer<QWaylandWindow> lw_window : send_property_window_list) {
            if (lw_window) {
                const QVariantMap &properites = lw_window->properties();

                // 当kwayland_shell被创建后，找到以_d_dwayland_开头的扩展属性将其设置一遍
                for (auto p = properites.constBegin(); p != properites.constEnd(); ++p) {
                    if (p.key().startsWith(QStringLiteral(_DWAYALND_)))
                        sendProperty(lw_window->shellSurface(), p.key(), p.value());
                }
            }
        }

        send_property_window_list.clear();
    });
    registry->setup();

    return shell;
}

}

#include "main.moc"
