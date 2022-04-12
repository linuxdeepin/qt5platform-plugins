/*
   * Copyright (C) 2022 Uniontech Software Technology Co.,Ltd.
   *
   * Author:     chenke <chenke@uniontech.com>
   *
   * Maintainer: chenke <chenke@uniontech.com>
   *
   * This program is free software: you can redistribute it and/or modify
   * it under the terms of the GNU Lesser General Public License as published by
   * the Free Software Foundation, either version 3 of the License, or
   * any later version.
   *
   * This program is distributed in the hope that it will be useful,
   * but WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.
   *
   * You should have received a copy of the GNU Lesser General Public License
   * along with this program.  If not, see <http://www.gnu.org/licenses/>.
   */
#include "dkeyboard.h"
#include "dwaylandshellmanagerv2.h"
#include "vtablehook.h"

#include <QGuiApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QScreen>
#include <QtWaylandClientVersion>

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformcursor.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qobject_p.h>
#include <private/qwaylandshellintegration_p.h>
#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <private/qwidgetwindow_p.h>
#include <wayland-client-core.h>

#include <KWayland/Client/ddeshell.h>
#include <KWayland/Client/ddeseat.h>
#include <KWayland/Client/ddekeyboard.h>
#include <KWayland/Client/strut.h>
#include <KWayland/Client/fakeinput.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/server_decoration.h>

#ifndef QT_DEBUG
Q_LOGGING_CATEGORY(dkwlp, "dtk.kwayland.plugin" , QtInfoMsg);
#else
Q_LOGGING_CATEGORY(dkwlp, "dtk.kwayland.plugin");
#endif

DPP_USE_NAMESPACE

namespace QtWaylandClient {

//刷新时间队列，等待kwin反馈消息
static void wlSync()
{
    auto res = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("display", nullptr);
    auto display = reinterpret_cast<wl_display *>(res);
    if (display) {
        wl_display_roundtrip(display);
    }
}

// 更新平台鼠标位置
static void pointerEvent(const QPointF &pointF, QEvent::Type type,
                         Qt::MouseButton button = Qt::NoButton,
                         Qt::MouseButtons buttons= Qt::NoButton,
                         Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    if (type != QEvent::MouseButtonPress && type != QEvent::Move && type != QEvent::MouseButtonRelease)
        return;

    for (QScreen *screen : qApp->screens()) {
        if (!screen || !screen->handle() || !screen->handle()->cursor())
            continue;

        // cursor()->pointerEvent 中只用到 event.globalPos(), 即 pointF 这个参数
        QMouseEvent event(type, QPointF(), QPointF(), pointF, button, buttons, modifiers);
        screen->handle()->cursor()->pointerEvent(event);
    }
}

static Qt::WindowStates getwindowStates(DDEShellSurface *surface)
{
    Qt::WindowStates state = Qt::WindowNoState;
    if (surface->isMinimized())
        state = Qt::WindowMinimized;
    else if (surface->isFullscreen())
        state = Qt::WindowFullScreen;
    else if (surface->isMaximized())
        state = Qt::WindowMaximized;

    return state;
}

class DWaylandShellManagerV2Private : public QObjectPrivate {
public:
    Q_DECLARE_PUBLIC(DWaylandShellManagerV2)

    DWaylandShellManagerV2Private(int ver = QObjectPrivateVersion)
        : QObjectPrivate (ver) {
    }

    inline wl_surface *wlSurface(QWaylandWindow *window)
    {
#if QTWAYLANDCLIENT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        return window->wlSurface();
#else
        return window->object();
#endif
    }

    void setCursorPos(QPointF pos);
    // PlasmaShellSurface to set role / setposition and so on
    PlasmaShellSurface *plasmaShellSurface(QWaylandShellSurface *wlShellSurface);
    DDEShellSurface *ddeShellSurface(QWaylandShellSurface *wlShellSurface);
    Strut *createStrut();

    void handleGeometryChange(QWaylandWindow *window);
    void handleWindowStateChanged(QWaylandWindow *window);
    bool isDssProperty(QWaylandWindow *window, const QString &name, const QVariant &value);
    bool isPssProperty(QWaylandWindow *window, const QString &name, const QVariant &value);
    void setDockStrut(QWaylandWindow *window, int data[4]);
    void pendingPostion(QWaylandWindow *window);

    Registry *registry = nullptr;
    PlasmaShell *ps = nullptr;
    ServerSideDecorationManager *ssdm = nullptr;
    DDEShell *ddeShell = nullptr;
    DDESeat *ddeSeat = nullptr;
    DDEPointer *ddePointer = nullptr;
    DDETouch *ddeTouch = nullptr;
    DDEKeyboard *ddeKeyboard = nullptr;
    Strut *strut = nullptr;
    FakeInput *fakeInput = nullptr;
    QList<QPointer<QWaylandWindow>> sendlist;

    bool isTouchMotion = false;
    QPointF lastTouchPos;
};

void DWaylandShellManagerV2Private::setCursorPos(QPointF pos)
{
    Q_Q(DWaylandShellManagerV2);
    q->createDDEFakeInput();

    if (!fakeInput) {
        qCWarning(dkwlp) << "fakeInput not create";
        return;
    }
    if (!fakeInput->isValid()) {
        qCWarning(dkwlp) << "fakeInput is invalid";
        return;
    }

    fakeInput->requestPointerMoveAbsolute(pos);
}

PlasmaShellSurface *DWaylandShellManagerV2Private::plasmaShellSurface(QWaylandShellSurface *wlShellSurface)
{
    if (!ps || !wlShellSurface) {
        qCCritical(dkwlp) << "PlasmaShell or wlShellSurface is null";
        return nullptr;
    }

    auto *pss = wlShellSurface->findChild<PlasmaShellSurface*>();

    if (pss)
        return pss;

    if (!wlShellSurface->window()) {
        qCCritical(dkwlp) << "QWaylandShellSurface window not created!";
        return nullptr;
    }

    return ps->createSurface(wlSurface(wlShellSurface->window()), wlShellSurface);
}

DDEShellSurface *DWaylandShellManagerV2Private::ddeShellSurface(QWaylandShellSurface *wlShellSurface)
{
    if (!ddeShell || !wlShellSurface) {
        qCCritical(dkwlp) << "ddeShell or wlShellSurface is null";
        return nullptr;
    }

    auto *dss = wlShellSurface->findChild<DDEShellSurface*>();

    if (dss)
        return dss;

    if (!wlShellSurface->window()) {
        qCCritical(dkwlp) << "QWaylandShellSurface window not created!";
        return nullptr;
    }

    return ddeShell->createShellSurface(wlSurface(wlShellSurface->window()), wlShellSurface);
}

void DWaylandShellManagerV2Private::handleWindowStateChanged(QWaylandWindow *window)
{
    auto surface = window->shellSurface();
    auto dss = ddeShellSurface(surface);
    if (!dss)
        return;

#define d_oldState QStringLiteral("_d_oldState")
    window->setProperty(d_oldState, Qt::WindowNoState);
#define STATE_CHANGED(sig) \
    QObject::connect(dss, &DDEShellSurface::sig, window, [window, dss](){\
        qCDebug(dkwlp) << "==== "#sig ;\
        const Qt::WindowStates &newState = getwindowStates(dss); \
        const int &oldState = window->property(d_oldState).toInt(); \
        QWindowSystemInterface::handleWindowStateChanged(window->window(), newState, oldState); \
        window->setProperty(d_oldState, static_cast<int>(newState)); \
    })

    STATE_CHANGED(minimizedChanged);
    STATE_CHANGED(maximizedChanged);
    STATE_CHANGED(fullscreenChanged);

    //    STATE_CHANGED(activeChanged);
    QObject::connect(dss, &DDEShellSurface::activeChanged, window, [window, dss](){
        QWindow *w = dss->isActive() ? window->window() : nullptr;
        QWindowSystemInterface::handleWindowActivated(w, Qt::FocusReason::ActiveWindowFocusReason);
    });

#define SYNC_FLAG(sig, enableFunc, flag) \
    QObject::connect(dss, &DDEShellSurface::sig, window, [window, dss](){ \
        qCDebug(dkwlp) << "==== "#sig << (enableFunc); \
        window->window()->setFlag(flag, enableFunc);\
    })

//    SYNC_FLAG(keepAboveChanged, ddeShellSurface->isKeepAbove(), Qt::WindowStaysOnTopHint);
    QObject::connect(dss, &DDEShellSurface::keepAboveChanged, window, [window, dss](){ \
        bool isKeepAbove = dss->isKeepAbove();
        qCDebug(dkwlp) << "==== keepAboveChanged" << isKeepAbove;
        window->window()->setFlag(Qt::WindowStaysOnTopHint, isKeepAbove);
        window->window()->setProperty("_d_dwayland_staysontop", isKeepAbove);
    });
    SYNC_FLAG(keepBelowChanged, dss->isKeepBelow(), Qt::WindowStaysOnBottomHint);
    SYNC_FLAG(minimizeableChanged, dss->isMinimizeable(), Qt::WindowMinimizeButtonHint);
    SYNC_FLAG(maximizeableChanged, dss->isMinimizeable(), Qt::WindowMaximizeButtonHint);
    SYNC_FLAG(closeableChanged, dss->isCloseable(), Qt::WindowCloseButtonHint);
    SYNC_FLAG(fullscreenableChanged, dss->isFullscreenable(), Qt::WindowFullscreenButtonHint);

    // TODO: not support yet
    //SYNC_FLAG(movableChanged, ddeShellSurface->isMovable(), Qt::??);
    //SYNC_FLAG(resizableChanged, ddeShellSurface->isResizable(), Qt::??);
    //SYNC_FLAG(acceptFocusChanged, ddeShellSurface->isAcceptFocus(), Qt::??);
    //SYNC_FLAG(modalityChanged, ddeShellSurface->isModal(), Qt::??);
}

bool DWaylandShellManagerV2Private::isDssProperty(QWaylandWindow *window, const QString &name, const QVariant &value)
{
    auto *dss = ddeShellSurface(window->shellSurface());
    if (!dss)
        return false;

    if (!name.compare(noTitlebar)) {
        qCDebug(dkwlp) << "### requestNoTitleBar" << value;
        dss->requestNoTitleBarProperty(value.toBool());
        return true;
    }

    if (!name.compare(windowRadius)) {
        bool ok = false;
        qreal radius  = value.toInt(&ok);
        if (window->window() && window->window()->screen())
            radius *= window->window()->screen()->devicePixelRatio();
        qCDebug(dkwlp) << "### requestWindowRadius" << radius << value;

        if (ok)
            dss->requestWindowRadiusProperty({radius, radius});
         else
            qCWarning(dkwlp) << "invalid property" << name << value;

        return true;
    }

    if (!name.compare(splitWindowOnScreen)) {
        bool ok = false;
        qreal leftOrRight  = value.toInt(&ok);
        if (ok) {
            dss->requestSplitWindow(DDEShellSurface::SplitType(leftOrRight));
            qCDebug(dkwlp) << "requestSplitWindow value: " << leftOrRight;
        } else {
            qCWarning(dkwlp) << "invalid property: " << name << value;
        }

        window->window()->setProperty(splitWindowOnScreen, 0);
        return true;
    }

    if (!name.compare(supportForSplittingWindow)) {
        if (window->window())
            window->window()->setProperty(supportForSplittingWindow, dss->isSplitable());

        return true;
    }

    if (QStringLiteral("_d_dwayland_staysontop") == name) {
        dss->requestKeepAbove(value.toBool());
        return true;
    }

    return false;
}

bool DWaylandShellManagerV2Private::isPssProperty(QWaylandWindow *window, const QString &name, const QVariant &value)
{
    if (!window)
        return false;

    auto *pss = plasmaShellSurface(window->shellSurface());
    // 如果创建失败则说明kwaylnd_shell对象还未初始化，应当终止设置
    // 记录下本次的设置行为，kwayland_shell创建后会重新设置这些属性
    if (!pss) {
        if (window)
            sendlist << window;
        return true;
    }

    // 将popup的窗口设置为tooltop层级, 包括qmenu，combobox弹出窗口
    if (window && window->window()->type() == Qt::Popup) {
        pss->setRole(PlasmaShellSurface::Role::ToolTip);
        return true;
    }

    // 禁止窗口移动接口适配。
    if (!name.compare(enableSystemMove)) {
        pss->setRole(value.toBool() ? PlasmaShellSurface::Role::Normal : PlasmaShellSurface::Role::StandAlone);
        return true;
    }

    if (QStringLiteral("_d_dwayland_window-position") == name) {
        pss->setPosition(value.toPoint());
        return true;
    }

    if (QStringLiteral("_d_dwayland_window-type") == name) {
        struct PropRole {
            QString value;
            PlasmaShellSurface::Role role;
        };

        static PropRole type2Role[] = {
            {"normal", PlasmaShellSurface::Role::Normal},

            {"desktop", PlasmaShellSurface::Role::Desktop},

            {"panel",PlasmaShellSurface::Role::Panel},
            {"dock",PlasmaShellSurface::Role::Panel},

            {"wallpaper",PlasmaShellSurface::Role::OnScreenDisplay},
            {"onScreenDisplay",PlasmaShellSurface::Role::OnScreenDisplay},

            {"notification",PlasmaShellSurface::Role::Notification},

            {"tooltip",PlasmaShellSurface::Role::ToolTip},

            {"launcher",PlasmaShellSurface::Role::StandAlone},
            {"standAlone",PlasmaShellSurface::Role::StandAlone},

            {"session-shell",PlasmaShellSurface::Role::Override},
            {"wallpaper-set",PlasmaShellSurface::Role::Override},
            {"override",PlasmaShellSurface::Role::Override},

            {"activeFullScreen",PlasmaShellSurface::Role::ActiveFullScreen},
        };

        const QString &winType = value.toString();
        for (uint i = 0; i< sizeof (type2Role) / sizeof (type2Role[0]); ++i) {
            if (!winType.compare(type2Role[i].value, Qt::CaseInsensitive)) {
                qCDebug(dkwlp) << "setRole" << type2Role[i].value << int(type2Role[i].role);
                pss->setRole(type2Role[i].role);
                return true;
            }
        }
    }

    return false;
}

void DWaylandShellManagerV2Private::setDockStrut(QWaylandWindow *window, int data[4])
{
    if (!createStrut())
        return;

    deepinKwinStrut dockStrut;
    switch (data[0]) {
    case 0:
        dockStrut.left = data[1];
        dockStrut.left_start_y = data[2];
        dockStrut.left_end_y = data[3];
        break;
    case 1:
        dockStrut.top = data[1];
        dockStrut.top_start_x = data[2];
        dockStrut.top_end_x = data[3];
        break;
    case 2:
        dockStrut.right = data[1];
        dockStrut.right_start_y = data[2];
        dockStrut.right_end_y = data[3];
        break;
    case 3:
        dockStrut.bottom = data[1];
        dockStrut.bottom_start_x = data[2];
        dockStrut.bottom_end_x = data[3];
        break;
    default:
        break;
    }
    if (strut)
        strut->setStrutPartial(wlSurface(window), dockStrut);
}

// 设置窗口位置, 默认都需要设置，同时判断如果窗口并没有移动过，则不需要再设置位置，而是由窗管默认平铺显示
void DWaylandShellManagerV2Private::pendingPostion(QWaylandWindow *window)
{
    bool bSetPosition = true;
    if (window->window()->inherits("QWidgetWindow")) {
        QWidgetWindow *widgetWin = static_cast<QWidgetWindow*>(window->window());
        if (widgetWin && widgetWin->widget()) {
            // 1. dabstractdialog 的 showevent 中会主动 move 到屏幕居中的位置, 即 setAttribute(Qt::WA_Moved)。
            // 2. 有 parent(ddialog dlg(this)) 的 window 窗管会主动调整位置，没有设置parent的才需要插件调整位置 如 ddialog dlg;
            bSetPosition = !window->transientParent() && widgetWin->widget()->testAttribute(Qt::WA_Moved);
        }
    }

    //QWaylandWindow对应surface的geometry，如果使用QWindow会导致缩放后surface坐标错误。
    if (bSetPosition)
        window->sendProperty("_d_dwayland_window-position", window->geometry().topLeft());
}

void DWaylandShellManagerV2Private::handleGeometryChange(QWaylandWindow *window)
{
    auto surface = window->shellSurface();
    auto dss = ddeShellSurface(surface);
    if (!dss)
        return;

    QObject::connect(dss, &DDEShellSurface::geometryChanged, [=] (const QRect &rc) {
        QRect rect = rc;
        rect.setWidth(window->geometry().width());
        rect.setHeight(window->geometry().height());
//        qCDebug(dkwlp()) << __func__ << "rc" << rc << "rect" << rect;
        QWindowSystemInterface::handleGeometryChange(window->window(), rect);
    });
}

DWaylandShellManagerV2::DWaylandShellManagerV2(QObject *parent)
    : QObject(*new DWaylandShellManagerV2Private, parent)
{

}

DWaylandShellManagerV2::~DWaylandShellManagerV2()
{

}

void DWaylandShellManagerV2::init(Registry *registry)
{
    Q_D(DWaylandShellManagerV2);
    if (d->registry == registry)
        return;

    d->registry = registry;
    connect(registry, &Registry::serverSideDecorationManagerAnnounced,
            this, &DWaylandShellManagerV2::createKWaylandSSD);

    connect(registry, &Registry::ddeShellAnnounced, this, &DWaylandShellManagerV2::createDDEShell);
    connect(registry, &Registry::plasmaShellAnnounced, this, &DWaylandShellManagerV2::createPlasmaShell);

    connect(registry, &Registry::ddeSeatAnnounced, this, &DWaylandShellManagerV2::createDDESeat);
//    connect(registry, &Registry::strutAnnounced, this, &DWaylandShellManagerV2::createStrut);
    connect(registry, &Registry::interfacesAnnounced, this, &DWaylandShellManagerV2::createDDEPointer);
    connect(registry, &Registry::interfacesAnnounced, this, &DWaylandShellManagerV2::createDDETouch);

    {
//       createDDEKeyboard();
//       createDDEFakeInput();
    };
}

QWaylandShellSurface *DWaylandShellManagerV2::createShellSurface(QWaylandShellIntegration *integration, QWaylandWindow *window)
{
    auto surface = VtableHook::callOriginalFun(integration, &QWaylandShellIntegration::createShellSurface, window);

    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::sendProperty,
                                 [this](QWaylandShellSurface *surface, const QString &name, const QVariant &value){
        VtableHook::callOriginalFun(surface, &QWaylandShellSurface::sendProperty, name, value);

        if (name.startsWith("_d_"))
            sendProperty(surface->window(), name, value);
    });

    VtableHook::overrideVfptrFun(surface, &QWaylandShellSurface::wantsDecorations, [](QWaylandShellSurface *)->bool const{
        return false;
    });

    VtableHook::overrideVfptrFun(window, &QWaylandWindow::setGeometry,
                                 [this](QWaylandWindow *window, const QRect &rect) {
        VtableHook::callOriginalFun(window, &QWaylandWindow::setGeometry, rect);

        if (window->QPlatformWindow::parent())
            return;

         d_func()->pendingPostion(window);
//        setPostion(window->shellSurface() ,rect.topLeft());
    });

    VtableHook::overrideVfptrFun(window, &QWaylandWindow::requestActivateWindow, [this](QWaylandWindow *window){
        VtableHook::originalFun(window, &QWaylandWindow::requestActivateWindow);

        Q_D(DWaylandShellManagerV2);
        if (!d->ddeShell || window)
            return;

        if (auto *dss = d->ddeShellSurface(window->shellSurface())) {
            dss->requestActive();
        }
    });

    VtableHook::overrideVfptrFun(window, &QWaylandWindow::frameMargins, [](QWaylandWindow *)->QMargins {
        /*插件把qt自身的标题栏去掉了，使用的是窗管的标题栏，但是qtwaylandwindow每次传递坐标给kwin的时候，
        * 都计算了（3, 30）的偏移，导致每次设置窗口属性的时候，窗口会下移,这个（3, 30）的偏移其实是qt自身
        * 标题栏计算的偏移量，我们uos桌面不能带入这个偏移量
        */
        return QMargins(0, 0, 0, 0);
    });

    Q_D(DWaylandShellManagerV2);
    if (d->ddeShell) {
        QObject::connect(window, &QWaylandWindow::shellSurfaceCreated, this, [=]() {
            d->handleGeometryChange(window);
            d->handleWindowStateChanged(window);
        });
    } else {
        qCDebug(dkwlp)<<"DDEShell creat failed!";
    }

    if (!window->window()) {
        qCWarning(dkwlp) << __func__ << "window->window() is null!";
        return surface;
    }

    //将拖拽图标窗口置顶，QShapedPixmapWindow是Qt中拖拽图标窗口专用类
    if (window->window()->inherits("QShapedPixmapWindow")) {
        setWindowStaysOnTop(window->shellSurface());
    }

    d->pendingPostion(window);

    if (window->window()) {
        for (const QByteArray &pname : window->window()->dynamicPropertyNames()) {
            if (Q_LIKELY(!pname.startsWith("_d_")))
                continue;
            // 将窗口自定义属性记录到wayland window property中
            window->sendProperty(pname, window->window()->property(pname.constData()));
        }
    }

    // 如果kwayland的server窗口装饰已转变完成，则为窗口创建边框
    if (d->ssdm)
        QObject::connect(window, &QWaylandWindow::shellSurfaceCreated, this, [this, window] {
            createServerDecoration(window);
        });

    return surface;
}

void DWaylandShellManagerV2::sendProperty(QWaylandWindow *window, const QString &name, const QVariant &value)
{
    Q_D(DWaylandShellManagerV2);
    // 某些应用程序(比如日历，启动器)调用此方法时 self为空，导致插件崩溃
    if(!window->shellSurface()) {
        return;
    }

    if (d->isDssProperty(window, name, value) || d->isPssProperty(window, name, value))
        return;

    if (QStringLiteral("_d_dwayland_global_keyevent") == name && value.toBool()) {
        // 只有关心全局键盘事件才连接, 并且随窗口销毁而断开
        if (!d->ddeKeyboard)
            createDDEKeyboard();

        if (!d->ddeKeyboard)
            return;

        DKeyboard *keyboard = new DKeyboard(window);
        QObject::connect(d->ddeKeyboard, &DDEKeyboard::keyChanged, keyboard, &DKeyboard::handleKeyEvent);
        QObject::connect(d->ddeKeyboard, &DDEKeyboard::modifiersChanged, keyboard, &DKeyboard::handleModifiersChanged);
        return;
    }

    if (QStringLiteral("_d_dwayland_dockstrut") == name) {
        int data[4] = {0};
        const auto &vars = value.toList();
        for (int i = 0; i < 4; ++i) {
            data[i] = vars.value(i).toInt();
        }
        d->setDockStrut(window, data);
    }
}

void DWaylandShellManagerV2::setPostion(QWaylandShellSurface *surface, const QPoint &pos)
{
    Q_D(DWaylandShellManagerV2);
    if (auto *pss = d->plasmaShellSurface(surface))
        pss->setPosition(pos);
}

void DWaylandShellManagerV2::setRole(QWaylandShellSurface *surface, int role)
{
    Q_D(DWaylandShellManagerV2);

    if (auto *pss = d->plasmaShellSurface(surface))
        pss->setRole(PlasmaShellSurface::Role(role));
}

void DWaylandShellManagerV2::setWindowStaysOnTop(QWaylandShellSurface *surface, bool ontop)
{
    Q_D(DWaylandShellManagerV2);

    if (auto *dss = d->ddeShellSurface(surface))
        dss->requestKeepAbove(ontop);
}

void DWaylandShellManagerV2::createPlasmaShell(quint32 name, quint32 version)
{
    Q_D(DWaylandShellManagerV2);
    if (!d->registry || d->ps)
        return;

    d->ps = d->registry->createPlasmaShell(name, version, d->registry->parent());
    if (!d->ps) {
        qCCritical(dkwlp) << "createPlasmaShell failed";
        return;
    }

    for (QPointer<QWaylandWindow> window : d->sendlist) {
        if (!window)
            continue;

        const QVariantMap &properites = window->properties();
        // 当kwayland_shell被创建后，找到以_d_dwayland_开头的扩展属性将其设置一遍
        for (auto p = properites.constBegin(); p != properites.constEnd(); ++p) {
            if (p.key().startsWith("_d_"))
                sendProperty(window, p.key(), p.value());
        }
    }

    d->sendlist.clear();
}

void DWaylandShellManagerV2::createKWaylandSSD(quint32 name, quint32 version)
{
    Q_D(DWaylandShellManagerV2);

    if (!d->registry)
        return;

    qCDebug(dkwlp) << "hasInterface ServerSideDecorationManager?" <<
               d->registry->hasInterface(Registry::Interface::ServerSideDecorationManager);

    d->ssdm = d->registry->createServerSideDecorationManager(name, version, d->registry->parent());

    if (!d->ssdm || !d->ssdm->isValid())
        qWarning(dkwlp()) << __func__ << "invalid ServerSideDecorationManager!";
}

void DWaylandShellManagerV2::createDDEShell(quint32 name, quint32 version)
{
    Q_D(DWaylandShellManagerV2);

    if (!d->registry || d->ddeShell)
        return;

    qCDebug(dkwlp) << "hasInterface DDEShell?" <<
               d->registry->hasInterface(Registry::Interface::DDEShell);

    d->ddeShell = d->registry->createDDEShell(name, version, d->registry->parent());

    if (!d->ddeShell || !d->ddeShell->isValid())
        qWarning(dkwlp()) << __func__ << "invalid ddeShell!";
}

void DWaylandShellManagerV2::createDDESeat(quint32 name, quint32 version)
{
    Q_D(DWaylandShellManagerV2);

    if (!d->registry)
        return;
    qCDebug(dkwlp) << "hasInterface DDESeat?" <<
               d->registry->hasInterface(Registry::Interface::DDESeat);

    d->ddeSeat = d->registry->createDDESeat(name, version, d->registry->parent());

    if (!d->ddeSeat || !d->ddeSeat->isValid())
        qWarning(dkwlp()) << __func__ << "invalid ddeSeat!";
}

Strut *DWaylandShellManagerV2Private::createStrut()
{
    if (!registry) {
        qCWarning(dkwlp) << "registry not init!";
        return nullptr;
    }

    if (strut)
        return strut;

    if (!registry->hasInterface(Registry::Interface::Strut)) {
        qCWarning(dkwlp) << "do not have Strut Interface ";
        return nullptr;
    }

    auto intfc = registry->interface(Registry::Interface::Strut);

    strut = registry->createStrut(intfc.name, intfc.version, registry->parent());

    if (!strut || strut->isValid())
        qWarning(dkwlp) << __func__ << "invalid strut!";

    return strut;
}

void DWaylandShellManagerV2::createDDEPointer()
{
    Q_D(DWaylandShellManagerV2);
    if (!d->ddeSeat)
        return;

    if (!d->ddeSeat->isValid()) {
        qWarning(dkwlp()) << __func__ << "invalid ddeseat!";
        return;
    }

    d->ddePointer = d->ddeSeat->createDDePointer(d->registry->parent());
    if (!d->ddePointer) {
        qCCritical(dkwlp) << "createDDePointer failed";
        return;
    }

    //向kwin发送获取全局坐标请求
    d->ddePointer->getMotion();

    wlSync();

    //更新一次全局坐标
    pointerEvent(d->ddePointer->getGlobalPointerPos(), QEvent::Move);

    // mouse move
    QObject::connect(d->ddePointer, &DDEPointer::motion, this, [d](const QPointF &posF) {
        if (d->isTouchMotion)
            return;

        pointerEvent(posF, QEvent::Move);
    });
}

void DWaylandShellManagerV2::createDDETouch()
{
    Q_D(DWaylandShellManagerV2);
    if (!d->ddeSeat || d->ddeTouch)
        return;

    if (!d->ddeSeat->isValid()) {
        qCCritical(dkwlp) << __func__ << "invalid ddeseat!";
        return;
    }

    d->ddeTouch = d->ddeSeat->createDDETouch(d->registry->parent());
    if (!d->ddeTouch) {
        qCCritical(dkwlp) << "createDDETouch failed";
        return;
    }

    QObject::connect(d->ddeTouch, &DDETouch::touchDown, [d] (int32_t kwaylandId, const QPointF &pos) {
        if (kwaylandId != 0) {
            qCWarning(dkwlp) << "invalid kwaylandId!" << kwaylandId;
            return;
        }

        d->lastTouchPos = pos;
        d->setCursorPos(pos);
        pointerEvent(pos, QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
    });

    QObject::connect(d->ddeTouch, &DDETouch::touchMotion, this, [d] (int32_t kwaylandId, const QPointF &pos) {
        if (kwaylandId != 0) {
            qCWarning(dkwlp) << "invalid kwaylandId!" << kwaylandId;
            return;
        }

        d->isTouchMotion = true;
        pointerEvent(pos, QEvent::Move, Qt::LeftButton, Qt::LeftButton);
        d->setCursorPos(pos);
        d->lastTouchPos = pos;
    });

    QObject::connect(d->ddeTouch, &DDETouch::touchUp, this, [=] (int32_t kwaylandId) {
        if (kwaylandId != 0) {
            qCWarning(dkwlp) << "invalid kwaylandId!" << kwaylandId;
            return;
        }

        // 和 motion 的最后一个位置相等, 无需再更新
        if (d->isTouchMotion) {
            d->isTouchMotion = false;
            return;
        }

        d->setCursorPos(d->lastTouchPos);
        pointerEvent(d->lastTouchPos, QEvent::MouseButtonRelease);
    });

}

void DWaylandShellManagerV2::createDDEKeyboard()
{
    Q_D(DWaylandShellManagerV2);
    if (!d->ddeSeat)
        return;

    d->ddeKeyboard = d->ddeSeat->createDDEKeyboard(d->registry->parent());
    if (!d->ddeKeyboard) {
        qCCritical(dkwlp) << "createDDEKeyboard failed";
        return;
    }

    //刷新时间队列，等待kwin反馈消息
    wlSync();
}

void DWaylandShellManagerV2::createDDEFakeInput()
{
    Q_D(DWaylandShellManagerV2);
    if (!d->registry)
        return;

    if (d->fakeInput) {
//        qCDebug(dkwlp()) << "fakeInput already created";
        return ;
    }

    if (!d->registry->hasInterface(Registry::Interface::FakeInput)) {
        qCWarning(dkwlp) << "do not have FakeInput Interface ";
        return ;
    }

    auto fi = d->registry->interface(Registry::Interface::FakeInput);
    d->fakeInput = d->registry->createFakeInput(fi.name, fi.version, this);

    if (!d->fakeInput || !d->fakeInput->isValid()) {
        qCWarning(dkwlp) << "fake input create failed.";
        return;
    }

    // 打开设置光标位置的开关
    d->fakeInput->authenticate("dtk", QString("set cursor pos"));
}

void DWaylandShellManagerV2::createServerDecoration(QWaylandWindow *window)
{
    Q_D(DWaylandShellManagerV2);
    // 通过窗口属性控制是否显示最小化和最大化按钮
    QWaylandShellSurface *surface = window->shellSurface();
    if (!surface) {
        qCWarning(dkwlp) << __func__ << "shellSurface is null";
        return;
    }

    auto *dss = d->ddeShellSurface(surface);
    if (dss) {
        if (!(window->window()->flags() & Qt::WindowMinimizeButtonHint))
            dss->requestMinizeable(false);

        if (!(window->window()->flags() & Qt::WindowMaximizeButtonHint))
            dss->requestMaximizeable(false);

        if ((window->window()->flags() & Qt::WindowStaysOnTopHint))
            dss->requestKeepAbove(true);

        if ((window->window()->flags() & Qt::WindowDoesNotAcceptFocus))
            dss->requestAcceptFocus(false);

        if (window->window()->modality() != Qt::NonModal)
            dss->requestModal(true);
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

    auto *wlSurface = d->wlSurface(window);

    if (!wlSurface)
        return;

    // 创建由kwin server渲染的窗口边框对象
    if (auto ssd = d->ssdm->create(wlSurface, surface)) {
        ssd->requestMode(ServerSideDecoration::Mode::Server);
    }
}

}
