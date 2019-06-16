/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
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
#include "dnotitlebarwindowhelper.h"
#include "vtablehook.h"
#include "utility.h"
#ifdef Q_OS_LINUX
#include "dxcbwmsupport.h"
#endif

#define protected public
#include <QWindow>
#undef protected
#include <QMouseEvent>

Q_DECLARE_METATYPE(QPainterPath)

DPP_BEGIN_NAMESPACE

QHash<const QWindow*, DNoTitlebarWindowHelper*> DNoTitlebarWindowHelper::mapped;

DNoTitlebarWindowHelper::DNoTitlebarWindowHelper(QWindow *window)
    : QObject(window)
{
    mapped[window] = this;

    updateClipPathFromProperty();
    updateFrameMaskFromProperty();
    updateWindowRadiusFromProperty();
    updateBorderWidthFromProperty();
    updateBorderColorFromProperty();
    updateShadowRadiusFromProperty();
    updateShadowOffsetFromProperty();
    updateShadowColorFromProperty();
    updateEnableSystemResizeFromProperty();
    updateEnableSystemMoveFromProperty();
    updateEnableBlurWindowFromProperty();
    updateWindowBlurAreasFromProperty();
    updateWindowBlurPathsFromProperty();
    updateAutoInputMaskByClipPathFromProperty();

    VtableHook::overrideVfptrFun(window, &QWindow::event, this, &DNoTitlebarWindowHelper::windowEvent);
}

DNoTitlebarWindowHelper::~DNoTitlebarWindowHelper()
{
    mapped.remove(qobject_cast<QWindow*>(parent()));
}

void DNoTitlebarWindowHelper::setWindowProperty(QWindow *window, const char *name, const QVariant &value)
{
    const QVariant &old_value = window->property(name);

    if (old_value == value)
        return;

    if (value.typeName() == QByteArray("QPainterPath")) {
        const QPainterPath &old_path = qvariant_cast<QPainterPath>(old_value);
        const QPainterPath &new_path = qvariant_cast<QPainterPath>(value);

        if (old_path == new_path) {
            return;
        }
    }

    window->setProperty(name, value);

    if (DNoTitlebarWindowHelper *self = mapped.value(window)) {
        QByteArray name_array(name);

        if (!name_array.startsWith("_d_"))
            return;

        // to upper
        name_array[3] = name_array.at(3) & ~0x20;

        const QByteArray slot_name = "update" + name_array.mid(3) + "FromProperty";

        if (!QMetaObject::invokeMethod(self, slot_name.constData(), Qt::DirectConnection)) {
            qWarning() << "Failed to update property:" << slot_name;
        }
    }
}

void DNoTitlebarWindowHelper::updateClipPathFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateFrameMaskFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateWindowRadiusFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateBorderWidthFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateBorderColorFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateShadowRadiusFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateShadowOffsetFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateShadowColorFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateEnableSystemResizeFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateEnableSystemMoveFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateEnableBlurWindowFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateWindowBlurAreasFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateWindowBlurPathsFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateAutoInputMaskByClipPathFromProperty()
{
    // TODO
}

bool DNoTitlebarWindowHelper::windowEvent(QEvent *event)
{
    QWindow *w = this->window();
    DNoTitlebarWindowHelper *self = mapped.value(w);
    quint32 winId = w->winId();
    bool is_mouse_move = event->type() == QEvent::MouseMove && static_cast<QMouseEvent*>(event)->buttons() == Qt::LeftButton;

    if (event->type() == QEvent::MouseButtonRelease) {
        self->m_windowMoving = false;
    }

    if (is_mouse_move && self->m_windowMoving) {
        updateMoveWindow(winId);
    }

    bool ret = VtableHook::callOriginalFun(w, &QWindow::event, event);

    if (is_mouse_move && !event->isAccepted()) {
        if (!self->m_windowMoving && self->isEnableSystemMove(winId)) {
            self->m_windowMoving = true;

            event->accept();
            startMoveWindow(winId);
        }
    }

    return ret;
}

bool DNoTitlebarWindowHelper::isEnableSystemMove(quint32 winId)
{
    if (!m_enableSystemMove)
        return false;

#ifdef Q_OS_LINUX
    quint32 hints = DXcbWMSupport::getMWMFunctions(Utility::getNativeTopLevelWindow(winId));

    return (hints == DXcbWMSupport::MWM_FUNC_ALL || hints & DXcbWMSupport::MWM_FUNC_MOVE);
#endif

    return true;
}

void DNoTitlebarWindowHelper::startMoveWindow(quint32 winId)
{
    Utility::startWindowSystemMove(winId);
}

void DNoTitlebarWindowHelper::updateMoveWindow(quint32 winId)
{
    Utility::updateMousePointForWindowMove(winId);
}

DPP_END_NAMESPACE
