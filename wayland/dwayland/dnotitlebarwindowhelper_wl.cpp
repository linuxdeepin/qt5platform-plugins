/*
 * Copyright (C) 2017 ~ 2019 Uniontech Technology Co., Ltd.
 *
 * Author:     WangPeng <wangpenga@uniontech.com>
 *
 * Maintainer: AlexOne  <993381@qq.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "dnotitlebarwindowhelper_wl.h"
#include "../../src/vtablehook.h"

#define protected public
#include <QWindow>
#undef protected
#include <QMouseEvent>
#include <QGuiApplication>
#include <QTimer>
#include <QMetaProperty>
#include <QScreen>
#include <qpa/qplatformwindow.h>

#define private public
#include "QtWaylandClient/private/qwaylandintegration_p.h"
#include "QtWaylandClient/private/qwaylandshellsurface_p.h"
#include "QtWaylandClient/private/qwaylandwindow_p.h"
#include "QtWaylandClient/private/qwaylandcursor_p.h"
#undef private

#include <private/qguiapplication_p.h>

DPP_BEGIN_NAMESPACE

QMap<const QWindow*, DNoTitlebarWlWindowHelper*> DNoTitlebarWlWindowHelper::mapped;

DNoTitlebarWlWindowHelper::DNoTitlebarWlWindowHelper(QWindow *window)
    : QObject(window)
    , m_window(window)
{
    // 不允许设置窗口为无边框的
    if (window->flags().testFlag(Qt::FramelessWindowHint)) {
        window->setFlag(Qt::FramelessWindowHint, false);
    }

    mapped[window] = this;

    updateEnableSystemMoveFromProperty();
}

DNoTitlebarWlWindowHelper::~DNoTitlebarWlWindowHelper()
{
    if (VtableHook::hasVtable(m_window)) {
        VtableHook::resetVtable(m_window);
    }

    mapped.remove(qobject_cast<QWindow*>(parent()));

    // TODO
    // if (m_window->handle()) { // 当本地窗口还存在时，移除设置过的窗口属性
    //     //! Utility::clearWindowProperty(m_windowID, Utility::internAtom(_DEEPIN_SCISSOR_WINDOW));
    //     DPlatformIntegration::clearNativeSettings(m_windowID);
    // }
}

void DNoTitlebarWlWindowHelper::setWindowProperty(QWindow *window, const char *name, const QVariant &value)
{
    const QVariant &old_value = window->property(name);

    if (old_value.isValid() && old_value == value)
        return;

    if (value.typeName() == QByteArray("QPainterPath")) {
        const QPainterPath &old_path = qvariant_cast<QPainterPath>(old_value);
        const QPainterPath &new_path = qvariant_cast<QPainterPath>(value);

        if (old_path == new_path) {
            return;
        }
    }

    if(!window)
        return;

    window->setProperty(name, value);

    if(window->handle()) {
        QtWaylandClient::QWaylandWindow *wl_window = static_cast<QtWaylandClient::QWaylandWindow *>(window->handle());

        if (wl_window->shellSurface())
            wl_window->sendProperty(name, value);
    }

    if (DNoTitlebarWlWindowHelper *self = mapped.value(window)) {

        QByteArray name_array(name);
        if (!name_array.startsWith("_d_"))
            return;

        // to upper
        name_array[3] = name_array.at(3) & ~0x20;

        const QByteArray slot_name = "update" + name_array.mid(3) + "FromProperty";

        if (self->metaObject()->indexOfSlot(slot_name + QByteArray("()")) < 0)
            return;

        if (!QMetaObject::invokeMethod(self, slot_name.constData(), Qt::DirectConnection)) {
            qWarning() << "Failed to update property:" << slot_name;
        }
    }
}

void DNoTitlebarWlWindowHelper::popupSystemWindowMenu(quintptr wid)
{
    QWindow *window = fromQtWinId(wid);
    if(!window || !window->handle())
        return;

    QtWaylandClient::QWaylandWindow *wl_window = static_cast<QtWaylandClient::QWaylandWindow *>(window->handle());
    if (!wl_window->shellSurface())
        return;

    if (QtWaylandClient::QWaylandShellSurface *s = wl_window->shellSurface()) {
        auto wl_integration = static_cast<QtWaylandClient::QWaylandIntegration *>(QGuiApplicationPrivate::platformIntegration());
        s->showWindowMenu(wl_integration->display()->defaultInputDevice());
    }
}

void DNoTitlebarWlWindowHelper::updateEnableSystemMoveFromProperty()
{
    const QVariant &v = m_window->property(enableSystemMove);

    m_enableSystemMove = !v.isValid() || v.toBool();

    if (m_enableSystemMove) {
        using namespace std::placeholders;
        auto hook = std::bind(&DNoTitlebarWlWindowHelper::windowEvent, _1, _2, this);
        VtableHook::overrideVfptrFun(m_window, &QWindow::event, hook);
    } else if (VtableHook::hasVtable(m_window)) {
        VtableHook::resetVfptrFun(m_window, &QWindow::event);
    }
}

bool DNoTitlebarWlWindowHelper::windowEvent(QWindow *w, QEvent *event, DNoTitlebarWlWindowHelper *self)
{
    // m_window 的 event 被 override 以后，在 windowEvent 里面获取到的 this 就成 m_window 了，
    // 而不是 DNoTitlebarWlWindowHelper，所以此处 windowEvent 改为 static 并传 self 进来
    bool is_mouse_move = event->type() == QEvent::MouseMove && static_cast<QMouseEvent*>(event)->buttons() == Qt::LeftButton;

    if (Q_UNLIKELY(event->type() == QEvent::MouseButtonRelease)) {
        self->m_windowMoving = false;
    }

    bool ret = VtableHook::callOriginalFun(w, &QWindow::event, event);

    // workaround for kwin: Qt receives no release event when kwin finishes MOVE operation,
    // which makes app hang in windowMoving state. when a press happens, there's no sense of
    // keeping the moving state, we can just reset ti back to normal.
    if (Q_UNLIKELY(event->type() == QEvent::MouseButtonPress)) {
        self->m_windowMoving = false;
    }

    if (Q_LIKELY(is_mouse_move && !event->isAccepted()
            && w->geometry().contains(static_cast<QMouseEvent*>(event)->globalPos()))) {
        if (Q_UNLIKELY(!self->m_windowMoving && self->isEnableSystemMove())) {
            self->m_windowMoving = true;

            event->accept();
            startMoveWindow(w);
        }
    }

    return ret;
}

bool DNoTitlebarWlWindowHelper::isEnableSystemMove(/*quint32 winId*/)
{
    return m_enableSystemMove;
}

void DNoTitlebarWlWindowHelper::startMoveWindow(QWindow *window)
{
    // QWaylandWindow::startSystemMove
    if (window && window->handle()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                static_cast<QtWaylandClient::QWaylandWindow *>(window->handle())->startSystemMove(QCursor::pos());
#endif
#if QT_VERSION > QT_VERSION_CHECK(5, 14, 9)
                static_cast<QtWaylandClient::QWaylandWindow *>(window->handle())->startSystemMove();
#endif
    }
}

DPP_END_NAMESPACE
