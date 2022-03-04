#define protected public
#include <qwindow.h>
#undef protected

#include "dwaylandshellmanager.h"
#include <QtWaylandClientVersion>
#include <QLoggingCategory>
#include "global.h"
#ifndef QT_DEBUG
Q_LOGGING_CATEGORY(dwlp, "dtk.wayland.plugin" , QtInfoMsg);
#else
Q_LOGGING_CATEGORY(dwlp, "dtk.wayland.plugin");
#endif

DPP_USE_NAMESPACE

#define _DWAYALND_ "_d_dwayland_"
#define CHECK_PREFIX(key) (key.startsWith(_DWAYALND_) || key.startsWith("_d_"))

namespace QtWaylandClient {

// kwayland中PlasmaShell的全局对象，用于使用kwayland中的扩展协议
static QPointer<KWayland::Client::PlasmaShell> kwayland_shell;

// 用于记录设置过以_DWAYALND_开头的属性，当kwyalnd_shell对象创建以后要使这些属性生效
static QList<QPointer<QWaylandWindow>> send_property_window_list;
// kwin合成器提供的窗口边框管理器
static QPointer<KWayland::Client::ServerSideDecorationManager> kwayland_ssd;
//创建ddeshell
static QPointer<KWayland::Client::DDEShell> ddeShell = nullptr;

//kwayland
static QPointer<KWayland::Client::DDESeat> kwayland_dde_seat;
static QPointer<KWayland::Client::DDETouch> kwayland_dde_touch;
static QPointer<KWayland::Client::DDEPointer> kwayland_dde_pointer;
static QPointer<KWayland::Client::Strut> kwayland_strut;
static QPointer<KWayland::Client::DDEKeyboard> kwayland_dde_keyboard;
static QPointer<KWayland::Client::FakeInput> kwayland_dde_fake_input;
static QPointer<QWaylandWindow> current_window;

inline static wl_surface *getWindowWLSurface(QWaylandWindow *window)
{
#if QTWAYLANDCLIENT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return window->wlSurface();
#else
    return window->object();
#endif
}

static KWayland::Client::PlasmaShellSurface* createKWayland(QWaylandWindow *window)
{
    if (!window)
        return nullptr;

    if (kwayland_shell) {
        auto surface = window->shellSurface();
        return kwayland_shell->createSurface(getWindowWLSurface(window), surface);
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

static KWayland::Client::DDEShellSurface* createDDESurface(QWaylandWindow *window)
{
    if (!window)
        return nullptr;

    if (ddeShell) {
        auto surface = window->shellSurface();
        return ddeShell->createShellSurface(getWindowWLSurface(window), surface);
    }

    return nullptr;
}

static KWayland::Client::DDEShellSurface *ensureDDEShellSurface(QWaylandShellSurface *self)
{
    if (!self)
        return nullptr;
    auto *dde_shell_surface = self->findChild<KWayland::Client::DDEShellSurface*>();

    if (!dde_shell_surface) {
        dde_shell_surface = createDDESurface(self->window());
    }

    return dde_shell_surface;
}

DWaylandShellManager::DWaylandShellManager()
{

}

DWaylandShellManager::~DWaylandShellManager()
{

}

void DWaylandShellManager::sendProperty(QWaylandShellSurface *self, const QString &name, const QVariant &value)
{
    // 某些应用程序(比如日历，启动器)调用此方法时 self为空，导致插件崩溃
    if(!self) {
        return;
    }

    if (!CHECK_PREFIX(name))
        return VtableHook::callOriginalFun(self, &QWaylandShellSurface::sendProperty, name, value);

    auto *ksurface = ensureKWaylandSurface(self);
    // 如果创建失败则说明kwaylnd_shell对象还未初始化，应当终止设置
    // 记录下本次的设置行为，kwayland_shell创建后会重新设置这些属性
    if (!ksurface) {
        QWaylandWindow * qw = self->window();
        if (qw) {
            send_property_window_list << self->window();
        }
        return;
    }

    auto *dde_shell_surface = ensureDDEShellSurface(self);
    if (dde_shell_surface) {
        if (!name.compare(noTitlebar)) {
            qCDebug(dwlp()) << "### requestNoTitleBar" << value;
            dde_shell_surface->requestNoTitleBarProperty(value.toBool());
        }
        if (!name.compare(windowRadius)) {
            bool ok = false;
            qreal radius  = value.toInt(&ok);
            if (self->window() && self->window()->screen())
                radius *= self->window()->screen()->devicePixelRatio();
            qCDebug(dwlp()) << "### requestWindowRadius" << radius << value;
            if (ok)
                dde_shell_surface->requestWindowRadiusProperty({radius, radius});
             else
                qCWarning(dwlp) << "invalid property" << name << value;
        }
    }

    // 将popup的窗口设置为tooltop层级, 包括qmenu，combobox弹出窗口
    {
        QWaylandWindow *wayland_window = self->window();
        if (wayland_window) {
            if (wayland_window->window()->type() == Qt::Popup)
                ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Override);
        }
    }

#ifdef D_DEEPIN_KWIN
    // 禁止窗口移动接口适配。
    typedef KWayland::Client::PlasmaShellSurface::Role KRole;
    if (!name.compare(enableSystemMove)) {
        ksurface->setRole(value.toBool() ? KRole::Normal : KRole::StandAlone);
        return;
    }

    if (QStringLiteral(_DWAYALND_ "global_keyevent") == name && value.toBool()) {
        current_window = self->window();
        // 只有关心全局键盘事件才连接, 并且随窗口销毁而断开
        QObject::connect(kwayland_dde_keyboard, &KWayland::Client::DDEKeyboard::keyChanged,
                         current_window, &DWaylandShellManager::handleKeyEvent,
                         Qt::ConnectionType::UniqueConnection);
    }

#endif

    if (QStringLiteral(_DWAYALND_ "dockstrut") == name) {
        setDockStrut(self, value);
    }

    if (QStringLiteral(_DWAYALND_ "window-position") == name) {
        QWaylandWindow *wayland_window = self->window();
        if (!wayland_window) {
            return;
        }
        ksurface->setPosition(value.toPoint());
    } else if (QStringLiteral(_DWAYALND_ "window-type") == name) {
        const QByteArray &type = value.toByteArray();

        if (type == "normal") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Normal);
        } else if (type == "desktop") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Desktop);
        }else if (type == "dock" || type == "panel") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Panel);
            ksurface->setPanelBehavior(KWayland::Client::PlasmaShellSurface::PanelBehavior::AlwaysVisible);
        } else if (type == "wallpaper" || type == "onScreenDisplay") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::OnScreenDisplay);
        } else if (type == "notification") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Notification);
        } else if (type == "tooltip") {
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::ToolTip);
        } else if (type == "launcher" || type == "standAlone") {
#ifdef D_DEEPIN_KWIN
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::StandAlone);
#endif
        } else if (type == "session-shell" || type == "menu" || type == "wallpaper-set" || type == "override") {
#ifdef D_DEEPIN_KWIN
            ksurface->setRole(KWayland::Client::PlasmaShellSurface::Role::Override);
#endif
        } else {

        }
    } else if (QStringLiteral(_DWAYALND_ "staysontop") == name) {
        setWindowStaysOnTop(self, value.toBool());
    }
}

void DWaylandShellManager::setGeometry(QPlatformWindow *self, const QRect &rect)
{
    VtableHook::callOriginalFun(self, &QPlatformWindow::setGeometry, rect);

    if (!self->QPlatformWindow::parent()) {
        if (auto lw_window = static_cast<QWaylandWindow*>(self)) {
            lw_window->sendProperty(QStringLiteral(_DWAYALND_ "window-position"), rect.topLeft());
        }
    }
}

void DWaylandShellManager::pointerEvent(const QPointF &pointF, QEvent::Type type)
{
    for (QScreen *screen : qApp->screens()) {
        if (screen && screen->handle() && screen->handle()->cursor()) {
            // cursor()->pointerEvent 中只用到 event.globalPos(), 即 pointF 这个参数
            if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease || type == QEvent::Move) {
                const QMouseEvent event(type, QPointF(), QPointF(), pointF, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                screen->handle()->cursor()->pointerEvent(event);
            }
        }
    }
}

QWaylandShellSurface *DWaylandShellManager::createShellSurface(QWaylandShellIntegration *self, QWaylandWindow *window)
{
    auto surface = VtableHook::callOriginalFun(self, &QWaylandShellIntegration::createShellSurface, window);

    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::sendProperty, DWaylandShellManager::sendProperty);
    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::wantsDecorations, DWaylandShellManager::disableClientDecorations);
    VtableHook::overrideVfptrFun(window, &QPlatformWindow::setGeometry, DWaylandShellManager::setGeometry);
    VtableHook::overrideVfptrFun(window, &QPlatformWindow::requestActivateWindow, DWaylandShellManager::requestActivateWindow);
    VtableHook::overrideVfptrFun(window, &QPlatformWindow::frameMargins, DWaylandShellManager::frameMargins);

    if (ddeShell) {
        QObject::connect(window, &QWaylandWindow::shellSurfaceCreated, [window] {
            handleGeometryChange(window);
            handleWindowStateChanged(window);
        });
    } else {
        qDebug()<<"====DDEShell creat failed";
    }
    // 设置窗口位置, 默认都需要设置，同时判断如果窗口并没有移动过，则不需要再设置位置，而是由窗管默认平铺显示
    bool bSetPosition = true;
    if (window->window()->inherits("QWidgetWindow")) {
        QWidgetWindow *widgetWin = static_cast<QWidgetWindow*>(window->window());
        if (widgetWin && widgetWin->widget()) {
            if (!widgetWin->widget()->testAttribute(Qt::WA_Moved)) {
                bSetPosition = false;
            }
            // TODO: 这里对dialog特殊处理，dialog不需要设置固定的位置，否则里面的坐标会发生偏移导致点击偏移
            // 但是这不是问题的根本原因，还需要进一步分析
            if (widgetWin->widget()->property("DAbstractDialog").toBool()) {
                bSetPosition = false;
            }

            // 如果子窗口为QMenu,将窗口设在为tooltip的role
            if (widgetWin->widget()->inherits("QMenu")) {
                window->sendProperty(_DWAYALND_ "window-type", "menu");
            }
        }
    }
    if (bSetPosition) {
        //QWaylandWindow对应surface的geometry，如果使用QWindow会导致缩放后surface坐标错误。
        window->sendProperty(_DWAYALND_ "window-position", window->geometry().topLeft());
    }

    if (window->window()) {
        for (const QByteArray &pname : window->window()->dynamicPropertyNames()) {
            if (Q_LIKELY(!CHECK_PREFIX(pname)))
                continue;
            // 将窗口自定义属性记录到wayland window property中
            window->sendProperty(pname, window->window()->property(pname.constData()));
        }

        //将拖拽图标窗口置顶，QShapedPixmapWindow是Qt中拖拽图标窗口专用类
        if (window->window()->inherits("QShapedPixmapWindow")) {
            window->sendProperty(QStringLiteral(_DWAYALND_ "staysontop"), true);
        }
    }

    // 如果kwayland的server窗口装饰已转变完成，则为窗口创建边框
    if (kwayland_ssd) {
        QObject::connect(window, &QWaylandWindow::shellSurfaceCreated, [window] {
            createServerDecoration(window);
        });
    } else {
        qDebug()<<"====kwayland_ssd creat failed";
    }

    return surface;
}

void DWaylandShellManager::createKWaylandShell(KWayland::Client::Registry *registry, quint32 name, quint32 version)
{
    kwayland_shell = registry->createPlasmaShell(name, version, registry->parent());

    Q_ASSERT_X(kwayland_shell, "PlasmaShell", "Registry create PlasmaShell  failed.");

    for (QPointer<QWaylandWindow> lw_window : send_property_window_list) {
        if (lw_window) {
            const QVariantMap &properites = lw_window->properties();
            // 当kwayland_shell被创建后，找到以_d_dwayland_开头的扩展属性将其设置一遍
            for (auto p = properites.constBegin(); p != properites.constEnd(); ++p) {
                if (CHECK_PREFIX(p.key()))
                    sendProperty(lw_window->shellSurface(), p.key(), p.value());
            }
        }
    }

    send_property_window_list.clear();
}

void DWaylandShellManager::createKWaylandSSD(KWayland::Client::Registry *registry, quint32 name, quint32 version)
{
    kwayland_ssd = registry->createServerSideDecorationManager(name, version, registry->parent());
    Q_ASSERT_X(kwayland_ssd, "ServerSideDecorationManager", "KWayland Registry ServerSideDecorationManager failed.");
}

void DWaylandShellManager::createDDEShell(KWayland::Client::Registry *registry, quint32 name, quint32 version)
{
    ddeShell = registry->createDDEShell(name, version, registry->parent());
    Q_ASSERT_X(ddeShell, "DDEShell", "Registry create DDEShell failed.");
}

void DWaylandShellManager::createDDESeat(KWayland::Client::Registry *registry, quint32 name, quint32 version)
{
    kwayland_dde_seat = registry->createDDESeat(name, version, registry->parent());
    Q_ASSERT_X(kwayland_dde_seat, "DDESeat", "Registry create DDESeat failed.");
}

/*
 * @brief createStrut  创建dock区域，仅dock使用
 * @param registry
 * @param name
 * @param version
 */
void DWaylandShellManager::createStrut(KWayland::Client::Registry *registry, quint32 name, quint32 version)
{
    kwayland_strut = registry->createStrut(name, version, registry->parent());
    Q_ASSERT_X(kwayland_strut, "strut", "Registry create strut failed.");
}

void DWaylandShellManager::handleKeyEvent(quint32 key, KWayland::Client::DDEKeyboard::KeyState state, quint32 time)
{
    if (current_window && current_window->window() && !current_window->isActive()) {
        QEvent::Type type = state == KWayland::Client::DDEKeyboard::KeyState::Pressed ? QEvent::KeyPress : QEvent::KeyRelease;
        qCDebug(dwlp) << __func__ << " key " << key << " state " << (int)state << " time " << time;
        QWindowSystemInterface::handleKeyEvent(current_window->window(), time, type, key, Qt::NoModifier, QString());
    }
}

void DWaylandShellManager::createDDEKeyboard(KWayland::Client::Registry *registry)
{
    //create dde keyboard
    Q_ASSERT(kwayland_dde_seat);

    kwayland_dde_keyboard = kwayland_dde_seat->createDDEKeyboard(registry->parent());
    Q_ASSERT(kwayland_dde_keyboard);

    //刷新时间队列，等待kwin反馈消息
    auto display = reinterpret_cast<wl_display *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("display", nullptr));
    if (display) {
        wl_display_roundtrip(display);
    }
}

void DWaylandShellManager::createDDEFakeInput(KWayland::Client::Registry *registry)
{
    kwayland_dde_fake_input = registry->createFakeInput(registry->interface(KWayland::Client::Registry::Interface::FakeInput).name,
                                                        registry->interface(KWayland::Client::Registry::Interface::FakeInput).version);
    if (!kwayland_dde_fake_input || !kwayland_dde_fake_input->isValid()) {
        qInfo() << "fake input create failed.";
        return;
    }
    // 打开设置光标位置的开关
    kwayland_dde_fake_input->authenticate("dtk", QString("set cursor pos"));
}

void DWaylandShellManager::createDDEPointer(KWayland::Client::Registry *registry)
{
    //create dde pointer
    Q_ASSERT(kwayland_dde_seat);

    if (!registry) {
        qCritical() << "registry is null";
        return;
    }

    kwayland_dde_pointer = kwayland_dde_seat->createDDePointer();
    Q_ASSERT(kwayland_dde_pointer);

    //向kwin发送获取全局坐标请求
    kwayland_dde_pointer->getMotion();

    //刷新时间队列，等待kwin反馈消息
    auto display = reinterpret_cast<wl_display *>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("display", nullptr));
    if (display) {
        wl_display_roundtrip(display);
    }

    //更新一次全局坐标
    pointerEvent(kwayland_dde_pointer->getGlobalPointerPos(), QEvent::Move);

    // mouse move
    static bool isTouchMotion = false;
    QObject::connect(kwayland_dde_pointer, &KWayland::Client::DDEPointer::motion,
            [] (const QPointF &posF) {
        if (isTouchMotion)
            return;
        pointerEvent(posF, QEvent::Move);
    });

    // 适配触屏
    // 1.25倍的缩放还是需要单独处理
    static QPointF releasePos;
    kwayland_dde_touch = kwayland_dde_seat->createDDETouch();
    QObject::connect(kwayland_dde_touch, &KWayland::Client::DDETouch::touchDown, [=] (int32_t kwaylandId, const QPointF &pos) {
        if (kwaylandId != 0) {
            return;
        }
        releasePos = pos;

        setCursorPoint(pos);
        pointerEvent(pos, QEvent::MouseButtonPress);
    });
    QObject::connect(kwayland_dde_touch, &KWayland::Client::DDETouch::touchMotion, [=] (int32_t kwaylandId, const QPointF &pos) {
        if (kwaylandId != 0) {
            return;
        }
        isTouchMotion = true;
        pointerEvent(pos, QEvent::Move);
        setCursorPoint(pos);
        releasePos = pos;
    });
    QObject::connect(kwayland_dde_touch, &KWayland::Client::DDETouch::touchUp, [=] (int32_t kwaylandId) {
        if (kwaylandId != 0) {
            return;
        }

        // 和 motion 的最后一个位置相等, 无需再更新
        if (isTouchMotion) {
            isTouchMotion = false;
            return;
        }

        setCursorPoint(releasePos);
        pointerEvent(releasePos, QEvent::MouseButtonRelease);
    });
}

void DWaylandShellManager::requestActivateWindow(QPlatformWindow *self)
{
    VtableHook::originalFun(self, &QPlatformWindow::requestActivateWindow);

    if (!self->QPlatformWindow::parent() && ddeShell) {
        auto qwayland_window = static_cast<QWaylandWindow *>(self);
        if (qwayland_window) {
            QWaylandShellSurface *q_shell_surface = qwayland_window->shellSurface();
            auto *dde_shell_surface = ensureDDEShellSurface(q_shell_surface);
            if (dde_shell_surface) {
                dde_shell_surface->requestActive();
            }
        }
    }
}

/*插件把qt自身的标题栏去掉了，使用的是窗管的标题栏，但是qtwaylandwindow每次传递坐标给kwin的时候，
* 都计算了（3, 30）的偏移，导致每次设置窗口属性的时候，窗口会下移,这个（3, 30）的偏移其实是qt自身
* 标题栏计算的偏移量，我们uos桌面不能带入这个偏移量
*/
QMargins DWaylandShellManager::frameMargins(QPlatformWindow *self)
{
    Q_UNUSED(self)

    return QMargins(0, 0, 0, 0);
}

void DWaylandShellManager::handleGeometryChange(QWaylandWindow *window)
{
    auto surface = window->shellSurface();
    auto ddeShellSurface = ensureDDEShellSurface(surface);
    if (ddeShellSurface) {
        QObject::connect(
            ddeShellSurface,
            &KWayland::Client::DDEShellSurface::geometryChanged,
            [=] (const QRect &geom) {
                QWindowSystemInterface::handleGeometryChange(
                    window->window(),
                    QRect(
                        geom.x(),
                        geom.y(),
                        window->geometry().width(),
                        window->geometry().height()
                    )
                );
            }
        );
    }
}

typedef KWayland::Client::DDEShellSurface KCDFace;
Qt::WindowStates getwindowStates(KCDFace *surface)
{
    Qt::WindowStates state = Qt::WindowNoState;
//    if (surface->isActive())
//        state |= Qt::WindowActive;
    if (surface->isFullscreen())
        state |= Qt::WindowFullScreen;
    if (surface->isMinimized())
        state |= Qt::WindowMinimized;
    if (surface->isMaximized())
        state |= Qt::WindowMaximized;

    return state;
}

void DWaylandShellManager::handleWindowStateChanged(QWaylandWindow *window)
{
    auto surface = window->shellSurface();
    auto ddeShellSurface = ensureDDEShellSurface(surface);
    if (!ddeShellSurface)
        return;

#define d_oldState QStringLiteral("_d_oldState")
    window->setProperty(d_oldState, Qt::WindowNoState);
#define STATE_CHANGED(sig) \
    QObject::connect(ddeShellSurface, &KCDFace::sig, window, [window, ddeShellSurface](){\
        qCDebug(dwlp) << "==== "#sig ;\
        const Qt::WindowStates &newState = getwindowStates(ddeShellSurface); \
        const int &oldState = window->property(d_oldState).toInt(); \
        QWindowSystemInterface::handleWindowStateChanged(window->window(), newState, oldState); \
        window->setProperty(d_oldState, static_cast<int>(newState)); \
    })

    STATE_CHANGED(minimizedChanged);
    STATE_CHANGED(maximizedChanged);
    STATE_CHANGED(fullscreenChanged);

    //    STATE_CHANGED(activeChanged);
    QObject::connect(ddeShellSurface, &KCDFace::activeChanged, window, [window, ddeShellSurface](){
        QWindow *w = ddeShellSurface->isActive() ? window->window() : nullptr;
        QWindowSystemInterface::handleWindowActivated(w, Qt::FocusReason::ActiveWindowFocusReason);
    });

#define SYNC_FLAG(sig, enableFunc, flag) \
    QObject::connect(ddeShellSurface, &KCDFace::sig, window, [window, ddeShellSurface](){ \
        qCDebug(dwlp) << "==== "#sig << (enableFunc); \
        window->window()->setFlag(flag, enableFunc);\
    })

    SYNC_FLAG(keepAboveChanged, ddeShellSurface->isKeepAbove(), Qt::WindowStaysOnTopHint);
    SYNC_FLAG(keepBelowChanged, ddeShellSurface->isKeepBelow(), Qt::WindowStaysOnBottomHint);
    SYNC_FLAG(minimizeableChanged, ddeShellSurface->isMinimizeable(), Qt::WindowMinimizeButtonHint);
    SYNC_FLAG(maximizeableChanged, ddeShellSurface->isMinimizeable(), Qt::WindowMaximizeButtonHint);
    SYNC_FLAG(closeableChanged, ddeShellSurface->isCloseable(), Qt::WindowCloseButtonHint);
    SYNC_FLAG(fullscreenableChanged, ddeShellSurface->isFullscreenable(), Qt::WindowFullscreenButtonHint);

    // TODO: not support yet
    //SYNC_FLAG(movableChanged, ddeShellSurface->isMovable(), Qt::??);
    //SYNC_FLAG(resizableChanged, ddeShellSurface->isResizable(), Qt::??);
    //SYNC_FLAG(acceptFocusChanged, ddeShellSurface->isAcceptFocus(), Qt::??);
    //SYNC_FLAG(modalityChanged, ddeShellSurface->isModal(), Qt::??);
}

/*
 * @brief setWindowStaysOnTop  设置窗口置顶
 * @param surface
 * @param state true:设置窗口置顶，false:取消置顶
 */
void DWaylandShellManager::setWindowStaysOnTop(QWaylandShellSurface *surface, const bool state)
{
    auto *dde_shell_surface = ensureDDEShellSurface(surface);
    if (dde_shell_surface) {
        dde_shell_surface->requestKeepAbove(state);
    }
}

/*
 * @brief setDockStrut  设置窗口有效区域
 * @param surface
 * @param var[0]:dock位置 var[1]:dock高度或者宽度 var[2]:start值 var[3]:end值
 */
void DWaylandShellManager::setDockStrut(QWaylandShellSurface *surface, const QVariant var)
{
    KWayland::Client::deepinKwinStrut dockStrut;
    switch (var.toList()[0].toInt()) {
    case 0:
        dockStrut.left = var.toList()[1].toInt();
        dockStrut.left_start_y = var.toList()[2].toInt();
        dockStrut.left_end_y = var.toList()[3].toInt();
        break;
    case 1:
        dockStrut.top = var.toList()[1].toInt();
        dockStrut.top_start_x = var.toList()[2].toInt();
        dockStrut.top_end_x = var.toList()[3].toInt();
        break;
    case 2:
        dockStrut.right = var.toList()[1].toInt();
        dockStrut.right_start_y = var.toList()[2].toInt();
        dockStrut.right_end_y = var.toList()[3].toInt();
        break;
    case 3:
        dockStrut.bottom = var.toList()[1].toInt();
        dockStrut.bottom_start_x = var.toList()[2].toInt();
        dockStrut.bottom_end_x = var.toList()[3].toInt();
        break;
    default:
        break;
    }

    kwayland_strut->setStrutPartial(getWindowWLSurface(surface->window()), dockStrut);
}

/*
 * @brief setCursorPoint  设置光标在屏幕中的绝对位置
 * @param pos
 */
void DWaylandShellManager::setCursorPoint(QPointF pos) {
    if (!kwayland_dde_fake_input) {
        qInfo() << "kwayland_dde_fake_input is nullptr";
        return;
    }
    if (!kwayland_dde_fake_input->isValid()) {
        qInfo() << "kwayland_dde_fake_input is invalid";
        return;
    }
    kwayland_dde_fake_input->requestPointerMoveAbsolute(pos);
}

bool DWaylandShellManager::disableClientDecorations(QWaylandShellSurface *surface)
{
    Q_UNUSED(surface)

    // 禁用qtwayland自带的bradient边框（太丑了）
    return false;
}

void DWaylandShellManager::createServerDecoration(QWaylandWindow *window)
{
    // 通过窗口属性控制是否显示最小化和最大化按钮
    QWaylandShellSurface *q_shell_surface = window->shellSurface();
    if (q_shell_surface) {
        auto *dde_shell_surface = ensureDDEShellSurface(q_shell_surface);
        if (dde_shell_surface) {
            if (!(window->window()->flags() & Qt::WindowMinimizeButtonHint)) {
                dde_shell_surface->requestMinizeable(false);
            }
            if (!(window->window()->flags() & Qt::WindowMaximizeButtonHint)) {
                dde_shell_surface->requestMaximizeable(false);
            }
            if ((window->window()->flags() & Qt::WindowStaysOnTopHint)) {
                dde_shell_surface->requestKeepAbove(true);
            }
            if ((window->window()->flags() & Qt::WindowDoesNotAcceptFocus)) {
                dde_shell_surface->requestAcceptFocus(false);
            }
            if (window->window()->modality() != Qt::NonModal) {
                dde_shell_surface->requestModal(true);
            }
        }
    }

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

    auto *surface = getWindowWLSurface(window);

    if (!surface)
        return;

    // 创建由kwin server渲染的窗口边框对象
    if (auto ssd = kwayland_ssd->create(surface, q_shell_surface)) {
        ssd->requestMode(KWayland::Client::ServerSideDecoration::Mode::Server);
    }
}

}
