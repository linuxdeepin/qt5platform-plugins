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

#include "dinputselectionhandle.h"
#include "ddesktopinputselectioncontrol.h"

#include <QImageReader>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>
#include <QMouseEvent>

DPP_BEGIN_NAMESPACE

DInputSelectionHandle::DInputSelectionHandle(HandlePosition position, QWindow *eventWindow, DDesktopInputSelectionControl *pControl)
    : QRasterWindow()
    , m_position(position)
    , m_eventWindow(eventWindow)
    , m_pInputSelectionControl(pControl)
{
    setFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);

    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    setFormat(format);
    updateImage(position);
}

DInputSelectionHandle::HandlePosition DInputSelectionHandle::handlePosition() const
{
    return m_position;
}

void DInputSelectionHandle::setHandlePosition(HandlePosition position)
{
    if (m_position == position) {
        return;
    }

    m_position = position;
    updateImage(position);
    update();
}

QSize DInputSelectionHandle::handleImageSize() const
{
    return m_image.size() / devicePixelRatio();
}

void DInputSelectionHandle::updateImage(HandlePosition position)
{
    QImage handle;
    QImageReader reader(position == Up ? ":/up_handle.svg" : ":/down_handle.svg");
    QSize image_size(reader.size());

    reader.setScaledSize(image_size * devicePixelRatio());
    reader.read(&handle);

    m_image = handle;
    m_image.setDevicePixelRatio(devicePixelRatio());
}

void DInputSelectionHandle::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe);
    QPainter painter(this);
    QImage image(m_image);
    const QSize szDelta = size() - image.size();

    QPainter pa(&image);
    pa.setCompositionMode(QPainter::CompositionMode_SourceIn);
    pa.fillRect(image.rect(), qApp->palette().highlight());

    // center image onto window
    painter.drawImage(QPointF(szDelta.width(), szDelta.height()) / 2, image);
}

void DInputSelectionHandle::mousePressEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(m_eventWindow, event);
}

void DInputSelectionHandle::mouseReleaseEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(m_eventWindow, event);
}

void DInputSelectionHandle::mouseMoveEvent(QMouseEvent *event)
{
    QCoreApplication::sendEvent(m_eventWindow, event);
}

DPP_END_NAMESPACE
