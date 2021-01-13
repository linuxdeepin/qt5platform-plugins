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

#ifndef DESKTOPINPUTSELECTIONCONTROL_H
#define DESKTOPINPUTSELECTIONCONTROL_H

#include "global.h"

#include <QObject>
#include <QSize>
#include <QPoint>
#include <QVector>
#include <QPointer>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QInputMethod;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class DInputSelectionHandle;
class DApplicationEventMonitor;
class DDesktopInputSelectionControl : public QObject
{
    Q_OBJECT

public:
    enum HandleState {
        HandleIsReleased = 0,
        HandleIsHeld = 1,
        HandleIsMoving = 2
    };

    enum HandleType {
        AnchorHandle = 0,
        CursorHandle = 1
    };

    DDesktopInputSelectionControl(QObject *parent, QInputMethod *inputMethod);
    ~DDesktopInputSelectionControl() override;

    void createHandles();
    void setApplicationEventMonitor(DApplicationEventMonitor *pMonitor);

public Q_SLOTS:
    void updateAnchorHandlePosition();
    void updateCursorHandlePosition();
    void updateVisibility();
    void onWindowStateChanged(Qt::WindowState state);
    void updateSelectionControlVisible();

signals:
    void anchorPositionChanged();
    void cursorPositionChanged();
    void anchorRectangleChanged();
    void cursorRectangleChanged();
    void selectionControlVisibleChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void setEnabled(bool val);
    void setHandleState(HandleState state);
    int anchorPosition() const;
    int cursorPosition() const;
    Qt::InputMethodHints inputMethodHints() const;
    QRectF anchorRectangle() const;
    QRectF cursorRectangle() const;
    QRectF keyboardRectangle() const;
    bool animating() const;
    bool selectionControlVisible() const;
    bool anchorRectIntersectsClipRect() const;
    bool cursorRectIntersectsClipRect() const;

    void setSelectionOnFocusObject(const QPointF &anchorPos, const QPointF &cursorPos);

    QRect anchorHandleRect() const;
    QRect cursorHandleRect() const;
    QRect handleRectForCursorRect(const QRectF &cursorRect) const;
    QRect handleRectForAnchorRect(const QRectF &anchorRect) const;

    bool testHandleVisible() const;

private:
    QInputMethod *m_pInputMethod;
    QScopedPointer<DInputSelectionHandle> m_anchorSelectionHandle;
    QScopedPointer<DInputSelectionHandle> m_cursorSelectionHandle;
    QPointer<DApplicationEventMonitor> m_pApplicationEventMonitor;
    QSize m_handleImageSize;

    HandleState m_handleState;
    HandleType m_currentDragHandle;
    bool m_anchorHandleVisible;
    bool m_cursorHandleVisible;
    bool m_eventFilterEnabled;
    bool m_selectionControlVisible;
    QPoint m_otherSelectionPoint;
    QVector<QMouseEvent *> m_eventQueue;
    QPoint m_distanceBetweenMouseAndCursor;
    QPoint m_handleDragStartedPosition;
    QSize m_fingerOptSize;
};

DPP_END_NAMESPACE

#endif // DESKTOPINPUTSELECTIONCONTROL_H
