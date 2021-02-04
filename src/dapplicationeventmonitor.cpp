/*
 * Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
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

#include "dapplicationeventmonitor.h"

#include <QGuiApplication>
#include <QInputEvent>
#include <QTouchDevice>

DPP_BEGIN_NAMESPACE

DApplicationEventMonitor::DApplicationEventMonitor(QObject *parent)
    : QObject(parent)
    , m_lastInputDeviceType(None)
{
    qApp->installEventFilter(this);
}

DApplicationEventMonitor::~DApplicationEventMonitor()
{
}

DApplicationEventMonitor::InputDeviceType DApplicationEventMonitor::lastInputDeviceType() const
{
    return m_lastInputDeviceType;
}

DApplicationEventMonitor::InputDeviceType DApplicationEventMonitor::eventType(QEvent *event)
{
    Q_ASSERT(event);

    InputDeviceType last_input_device_type = None;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick: {
        QMouseEvent *pMouseEvent = static_cast<QMouseEvent *>(event);

        if (pMouseEvent->source() == Qt::MouseEventNotSynthesized) { //由真实鼠标事件生成
            last_input_device_type = Mouse;
        }
        break;
    }
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    case QEvent::TabletMove:
        last_input_device_type = Tablet;
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        last_input_device_type = Keyboard;
        break;
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        QTouchEvent *pTouchEvent = static_cast<QTouchEvent *>(event);

        if (pTouchEvent->device()->type() == QTouchDevice::TouchScreen) {
            last_input_device_type = TouchScreen;
        }
        break;
    }
    default:
        break;
    }

    return last_input_device_type;
}

bool DApplicationEventMonitor::eventFilter(QObject *watched, QEvent *event)
{
    auto last_input_device_type = eventType(event);

    if (last_input_device_type != None && last_input_device_type != m_lastInputDeviceType) {
        m_lastInputDeviceType = last_input_device_type;
        Q_EMIT lastInputDeviceTypeChanged();
    }

    return QObject::eventFilter(watched, event);
}

DPP_END_NAMESPACE
