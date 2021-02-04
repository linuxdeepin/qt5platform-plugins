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

#ifndef INPUTSELECTIONHANDLE_H
#define INPUTSELECTIONHANDLE_H

#include "global.h"

#include <QRasterWindow>
#include <QImage>

QT_BEGIN_NAMESPACE
class QWindow;
class QMouseEvent;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class DDesktopInputSelectionControl;
class DInputSelectionHandle : public QRasterWindow
{
    Q_OBJECT

public:
    enum HandlePosition {
        Up = 0, //handle位于选中区域的上方
        Down = 1 // handle位于选中区域的下方
    };

    DInputSelectionHandle(HandlePosition position, DDesktopInputSelectionControl *pControl);

    HandlePosition handlePosition() const;
    void setHandlePosition(HandlePosition position);
    QSize handleImageSize() const;

protected:
    void paintEvent(QPaintEvent *pe) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void updateImage(HandlePosition position);

private:
    HandlePosition m_position;
    DDesktopInputSelectionControl *m_pInputSelectionControl;
    QImage m_image;
};

DPP_END_NAMESPACE

#endif // INPUTSELECTIONHANDLE_H
