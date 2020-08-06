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
#include <KWayland/Client/server_decoration.h>

#include <dshellsurface.h>

#include <QGuiApplication>
#include <QPointer>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

// 用于窗口设置和dwayland相关的特殊属性的前缀
// 以_d_dwayland_开头的属性需要做特殊处理，一般是用于和kwayland的交互
#define _DWAYALND_ "_d_dwayland_"

DPP_USE_NAMESPACE
using namespace DWaylandClient;

namespace QtWaylandClient {
// kwayland中PlasmaShell的全局对象，用于使用kwayland中的扩展协议
static QPointer<KWayland::Client::PlasmaShell> kwayland_shell;
// 用于记录设置过以_DWAYALND_开头的属性，当kwyalnd_shell对象创建以后要使这些属性生效
static QList<QPointer<QWaylandWindow>> send_property_window_list;
// kwin合成器提供的窗口边框管理器
static QPointer<KWayland::Client::ServerSideDecorationManager> kwayland_ssd;
// 在kwayland_ssd创建之前记录下已经创建的窗口
static QList<QPointer<QWaylandWindow>> wayland_window_list;
// dde wayland协议的manager
static QPointer<DShellSurfaceManager> dde_wayland;

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

static KWayland::Client::PlasmaShellSurface *ensureKWaylandSurface(QWaylandShellSurface *self)
{
    auto *ksurface = self->findChild<KWayland::Client::PlasmaShellSurface*>();

    if (!ksurface) {
        ksurface = createKWayland(self->window());
    }

    return ksurface;
}

static QMargins windowFrameMargins(QPlatformWindow *window)
{
    QWaylandWindow *wayland_window = static_cast<QWaylandWindow*>(window);

    if (DShellSurface *s = dde_wayland->ensureShellSurface(wayland_window->object())) {
        const QVariant &value= s->property("frameMargins");
        if (value.type() == QVariant::Rect) {
            const QRect &frame_margins = value.toRect();
            return QMargins(frame_margins.left(), frame_margins.top(), frame_margins.right(), frame_margins.bottom());
        }
    }

    return wayland_window->QPlatformWindow::frameMargins();
}

static void sendProperty(QWaylandShellSurface *self, const QString &name, const QVariant &value)
{
    if (!name.startsWith(QStringLiteral(_DWAYALND_)))
        return VtableHook::callOriginalFun(self, &QWaylandShellSurface::sendProperty, name, value);

    auto *ksurface = ensureKWaylandSurface(self);
    // 如果创建失败则说明kwaylnd_shell对象还未初始化，应当终止设置
    // 记录下本次的设置行为，kwayland_shell创建后会重新设置这些属性
    if (!ksurface) {
        send_property_window_list << self->window();
        return;
    }

    if (QStringLiteral(_DWAYALND_ "window-position") == name) {
        QWaylandWindow *wayland_window = self->window();

        // 如果窗口没有使用内置的标题栏，则在设置窗口位置时要考虑窗口管理器为窗口添加的标题栏
        if (!wayland_window->decoration()) {
            const QMargins &margins = windowFrameMargins(wayland_window);
            ksurface->setPosition(value.toPoint() - QPoint(margins.left(), margins.top()));
        } else {
            ksurface->setPosition(value.toPoint());
        }
    } else if (QStringLiteral(_DWAYALND_ "window-type") == name) {
        const QByteArray &type = value.toByteArray();

        if (type == "dock") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
            ksurface->setPanelBehavior(KWayland::Client::PlasmaShellSurface::PanelBehavior::AlwaysVisible);
        } else if (type == "launcher") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::StandAlone);
            ksurface->setPanelBehavior(KWayland::Client::PlasmaShellSurface::PanelBehavior::AlwaysVisible);
        } else if (type == "session-shell") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Override);
        } else if (type == "tooltip") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::ToolTip);
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

static bool disableClientDecorations(QWaylandShellSurface *surface)
{
    Q_UNUSED(surface)

    // 禁用qtwayland自带的bradient边框（太丑了）
    return false;
}

static void createServerDecoration(QWaylandWindow *window)
{
    bool decoration = false;
    switch (window->window()->type()) {
        case Qt::Window:
        case Qt::Widget:
        case Qt::Dialog:
        case Qt::Tool:
        case Qt::Drawer:
            decoration = true;
            break;
        default:
            break;
    }
    if (window->window()->flags() & Qt::FramelessWindowHint)
        decoration = false;
    if (window->window()->flags() & Qt::BypassWindowManagerHint)
        decoration = false;

    if (!decoration)
        return;

    auto *surface = window->object();

    if (!surface)
        return;

    // 创建由kwin server渲染的窗口边框对象
    auto ssd = kwayland_ssd->create(surface);
    ssd->requestMode(KWayland::Client::ServerSideDecoration::Mode::Server);
}

static QRect windowGeometry(QPlatformWindow *window)
{
    QWaylandWindow *wayland_window = static_cast<QWaylandWindow*>(window);
    QRect rect = wayland_window->QPlatformWindow::geometry();

    // 纠正窗口位置
    if (DShellSurface *s = dde_wayland->ensureShellSurface(wayland_window->object())) {
        rect.moveTopLeft(s->geometry().topLeft());

        if (!wayland_window->decoration()) {
            const QMargins &margins = windowFrameMargins(wayland_window);
            rect.translate(margins.left(), margins.top());
        }
    }

    return rect;
}

static void registerWidnowForDDESurface(DShellSurface *surface)
{
    if (!surface->window())
        return;

    auto wayland_window = static_cast<QWaylandWindow*>(surface->window()->handle());
    // 覆盖窗口geometry
    VtableHook::overrideVfptrFun(wayland_window, &QPlatformWindow::geometry, windowGeometry);
    // 没有使用内置边框的窗口应当从窗管获取frameMargins
    // 暂时不要开启此逻辑，qtwayland 5.11中假定窗口无外边框，不能正常处理frameMargins
//    if (!wayland_window->decoration())
//        VtableHook::overrideVfptrFun(wayland_window, &QPlatformWindow::frameMargins, windowFrameMargins);
    QObject::connect(surface, &DShellSurface::geometryChanged, surface, [surface] () {
        QWindowSystemInterface::handleGeometryChange(surface->window(), surface->window()->handle()->geometry());
    });
}

static QWaylandShellSurface *createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window)
{
    auto surface = VtableHook::callOriginalFun(self, &QWaylandShellIntegration::createShellSurface, window);

    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::sendProperty, sendProperty);
    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::wantsDecorations, disableClientDecorations);
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

    // 如果kwayland的server窗口装饰已转变完成，则为窗口创建边框
    if (kwayland_ssd) {
        createServerDecoration(window);
    } else {
        // 记录下来，等待kwayland的对象创建完成后为其创建边框
        wayland_window_list << window;
    }

    if (dde_wayland) {
        //  注册使用dde wayland的接口
        auto s = dde_wayland->registerWindow(window->window());

        if (s) {
            registerWidnowForDDESurface(s);
        } else {
            QObject::connect(dde_wayland, &DShellSurfaceManager::surfaceCreated, window, registerWidnowForDDESurface);
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
    connect(registry, &KWayland::Client::Registry::serverSideDecorationManagerAnnounced,
            this, [registry] (quint32 name, quint32 version) {
        kwayland_ssd = registry->createServerSideDecorationManager(name, version, registry->parent());

        for (auto w : wayland_window_list) {
            if (!w)
                continue;
            // 为窗口创建标题栏
            createServerDecoration(w);
        }

        wayland_window_list.clear();
    });

    // 窗口dde wayland的管理接口
    dde_wayland = new DShellSurfaceManager(this);
    registry->setup();

    return shell;
}

}

#include "main.moc"
