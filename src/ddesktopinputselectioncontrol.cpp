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

#include "ddesktopinputselectioncontrol.h"
#include "dinputselectionhandle.h"
#include "dapplicationeventmonitor.h"
#include "dselectedtexttooltip.h"

#include <qpa/qplatforminputcontext.h>
#include <QInputMethod>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QStyleHints>
#include <QImageReader>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

DPP_BEGIN_NAMESPACE

DDesktopInputSelectionControl::DDesktopInputSelectionControl(QObject *parent, QInputMethod *inputMethod)
    : QObject(parent)
    , m_pInputMethod(inputMethod)
    , m_anchorSelectionHandle()
    , m_cursorSelectionHandle()
    , m_handleState(HandleIsReleased)
    , m_eventFilterEnabled(true)
    , m_anchorHandleVisible(false)
    , m_cursorHandleVisible(false)
    , m_fingerOptSize(40, static_cast<int>(40 * 1.12)) // because a finger patch is slightly taller than its width
    , m_tooptipVisibleFlags(false)
{
    if (auto window = QGuiApplication::focusWindow()) {
        window->installEventFilter(this);
    }

    connect(m_pInputMethod, &QInputMethod::anchorRectangleChanged, this, [ = ]{
        QPointF point = anchorRectangle().topLeft();
        QObject *widget = QGuiApplication::focusObject();

        if (point.isNull() || (m_focusWindow.value(widget) == point)) {
            return;
        }

        m_tooptipVisibleFlags = false;
        m_focusWindow[widget] = point;
        widget->installEventFilter(this);
    });

    connect(m_pInputMethod, &QInputMethod::cursorRectangleChanged, this, &DDesktopInputSelectionControl::selectionControlVisibleChanged);

    connect(m_pInputMethod, &QInputMethod::anchorRectangleChanged, this, &DDesktopInputSelectionControl::anchorRectangleChanged);
    connect(m_pInputMethod, &QInputMethod::cursorRectangleChanged, this, &DDesktopInputSelectionControl::cursorRectangleChanged);

    connect(m_pInputMethod, &QInputMethod::cursorRectangleChanged, this, &DDesktopInputSelectionControl::cursorPositionChanged);
    connect(m_pInputMethod, &QInputMethod::anchorRectangleChanged, this, &DDesktopInputSelectionControl::anchorPositionChanged);

    connect(this, &DDesktopInputSelectionControl::selectionControlVisibleChanged, this, &DDesktopInputSelectionControl::updateSelectionControlVisible);

    connect(qApp, &QGuiApplication::focusWindowChanged, this, &DDesktopInputSelectionControl::onFocusWindowChanged);
}

DDesktopInputSelectionControl::~DDesktopInputSelectionControl()
{
    qDeleteAll(m_eventQueue);
    m_eventQueue.clear();
}

/*
 * Includes the hit area surrounding the visual handle
 */
QRect DDesktopInputSelectionControl::handleRectForCursorRect(const QRectF &cursorRect) const
{
    const int topMargin = (m_fingerOptSize.height() - m_handleImageSize.height()) / 2;
    QPoint pos(int(cursorRect.x() + (cursorRect.width() - m_fingerOptSize.width()) / 2),
               int(cursorRect.bottom()) - topMargin);

    // 当从后往前选中时，为了防止光标被文字覆盖，这里把光标绘制在顶部
    if (cursorRectangle().y() < anchorRectangle().y()) {
        pos.setY(int(cursorRect.top()) - topMargin - m_handleImageSize.height());
        //由于顶部光标和底部的光标不同，这里更新一下handle类型
        m_cursorSelectionHandle->setHandlePosition(DInputSelectionHandle::Up);
    } else {
        m_cursorSelectionHandle->setHandlePosition(DInputSelectionHandle::Down);
    }

    return QRect(pos, m_fingerOptSize);
}

QRect DDesktopInputSelectionControl::handleRectForAnchorRect(const QRectF &anchorRect) const
{
    const int topMargin = (m_fingerOptSize.height() - m_handleImageSize.height()) / 2;
    QPoint pos(int(anchorRect.x() + (anchorRect.width() - m_fingerOptSize.width()) / 2),
               int(anchorRect.top()) - topMargin - m_handleImageSize.height());

    // 当从后往前选中时，把anchor光标绘制底部，和cursor的处理保持一致
    if (cursorRectangle().y() < anchorRectangle().y()) {
        pos.setY(int(anchorRect.bottom()) - topMargin);
        m_anchorSelectionHandle->setHandlePosition(DInputSelectionHandle::Down);
    } else {
        m_anchorSelectionHandle->setHandlePosition(DInputSelectionHandle::Up);
    }

    return QRect(pos, m_fingerOptSize);
}

bool DDesktopInputSelectionControl::testHandleVisible() const
{
    return m_pApplicationEventMonitor->lastInputDeviceType() == DApplicationEventMonitor::TouchScreen;
}

/*
 * Includes the hit area surrounding the visual handle
 */
QRect DDesktopInputSelectionControl::anchorHandleRect() const
{
    return handleRectForAnchorRect(anchorRectangle());
}

/*
 * Includes the hit area surrounding the visual handle
 */
QRect DDesktopInputSelectionControl::cursorHandleRect() const
{
    return handleRectForCursorRect(cursorRectangle());
}

void DDesktopInputSelectionControl::updateAnchorHandlePosition()
{
    if (anchorRectangle().topLeft().isNull()) {
        m_anchorSelectionHandle->hide();
        return;
    }

    if (QWindow *focusWindow = QGuiApplication::focusWindow()) {
        const QPoint pos = focusWindow->mapToGlobal(anchorHandleRect().topLeft());
        m_anchorSelectionHandle->setPosition(pos);
    }
}

void DDesktopInputSelectionControl::updateCursorHandlePosition()
{
    if (anchorRectangle().topLeft().isNull()) {
        m_cursorSelectionHandle->hide();
        return;
    }

    if (QWindow *focusWindow = QGuiApplication::focusWindow()) {
        const QPoint pos = focusWindow->mapToGlobal(cursorHandleRect().topLeft());
        m_cursorSelectionHandle->setPosition(pos);
    }
}

void DDesktopInputSelectionControl::updateTooltipPosition()
{
    if (anchorRectangle().topLeft().isNull()) {
        m_selectedTextTooltip->hide();
        return;
    }

    if (QWindow *focusWindow = QGuiApplication::focusWindow()) {
        QPoint topleft_point;
        QSize tooltip_size = m_selectedTextTooltip->size();

        if (cursorRectangle().x() > anchorRectangle().x()) { // 从前往后选择
            auto tmp_point = focusWindow->mapToGlobal(anchorHandleRect().topLeft());
            topleft_point = QPoint(tmp_point.x() + m_fingerOptSize.width() / 2, tmp_point.y() - tooltip_size.height());
        } else { //从后往前选择
            auto tmp_point = focusWindow->mapToGlobal(anchorHandleRect().bottomLeft());
            topleft_point = QPoint(tmp_point.x() - m_fingerOptSize.width() / 2 - tooltip_size.width(), tmp_point.y() + tooltip_size.height());
        }

        if (topleft_point.x() < 0) {
            topleft_point.setX(m_fingerOptSize.width() / 2);
        }

        if (topleft_point.y() < 0) {
            auto margins = 0;
            if (m_anchorSelectionHandle->isVisible()) {
                auto anchorPosY = m_anchorSelectionHandle->position().y();
                auto cursorPosY =  m_cursorSelectionHandle->position().y();
                margins = anchorPosY > cursorPosY ? anchorPosY : cursorPosY;
            }

            topleft_point.setY(margins + tooltip_size.height());
        }

        m_selectedTextTooltip->setPosition(topleft_point);
    }
}

void DDesktopInputSelectionControl::createHandles()
{
    m_selectedTextTooltip.reset(new DSelectedTextTooltip);
    m_anchorSelectionHandle.reset(new DInputSelectionHandle(DInputSelectionHandle::Up, this));
    m_cursorSelectionHandle.reset(new DInputSelectionHandle(DInputSelectionHandle::Down, this));
    m_handleImageSize = m_anchorSelectionHandle->handleImageSize();

    m_anchorSelectionHandle->resize(m_fingerOptSize);
    m_cursorSelectionHandle->resize(m_fingerOptSize);
    connect(m_selectedTextTooltip.data(), &DSelectedTextTooltip::optAction,
            this, &DDesktopInputSelectionControl::onOptAction);
}

void DDesktopInputSelectionControl::onWindowStateChanged(Qt::WindowState state)
{
    m_focusWindow.clear();
    m_anchorSelectionHandle->setVisible(state != Qt::WindowState::WindowMinimized);
    m_cursorSelectionHandle->setVisible(state != Qt::WindowState::WindowMinimized);
    m_selectedTextTooltip->setVisible(state != Qt::WindowState::WindowMinimized);
}

void DDesktopInputSelectionControl::updateSelectionControlVisible()
{
    Q_ASSERT(m_pApplicationEventMonitor);

    bool selectText = m_pInputMethod->queryFocusObject(Qt::ImCurrentSelection,true).toString().isNull();

    if (!selectText) {
        setEnabled(true);
        m_anchorSelectionHandle->show();
        m_cursorSelectionHandle->show();
        m_selectedTextTooltip->show();
        updateAnchorHandlePosition();
        updateCursorHandlePosition();
        updateTooltipPosition();
    } else {
        setEnabled(false);
        m_anchorSelectionHandle->hide();
        m_cursorSelectionHandle->hide();
        if (m_tooptipVisibleFlags) {
            updateHandleFlags();
            return;
        }
        m_selectedTextTooltip->hide();
    }
    m_tooptipVisibleFlags = false;
    updateHandleFlags();
}

void DDesktopInputSelectionControl::onOptAction(int type)
{
    switch (type) {
    case DSelectedTextTooltip::Cut: {
        QKeyEvent key_event(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &key_event);
        break;
    }

    case DSelectedTextTooltip::Copy: {
        QKeyEvent key_event(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &key_event);
        Q_EMIT selectionControlVisibleChanged();
        break;
    }

    case DSelectedTextTooltip::Paste: {
        QKeyEvent key_event(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &key_event);
        break;
    }

    case DSelectedTextTooltip::SelectAll: {
        QKeyEvent key_event(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
        QCoreApplication::sendEvent(QGuiApplication::focusObject(), &key_event);
        // 更新handle的位置
        updateSelectionControlVisible();
        break;
    }
    default:
        break;
    }
}

void DDesktopInputSelectionControl::onFocusWindowChanged()
{
    if (!qApp->focusWindow()) {
        m_anchorSelectionHandle->hide();
        m_cursorSelectionHandle->hide();
        m_selectedTextTooltip->hide();
        m_focusWindow.clear();
    }
}

void DDesktopInputSelectionControl::setEnabled(bool val)
{
    // This will typically be set when a input field gets focus (and having selection).
    if (val) {
        connect(this, &DDesktopInputSelectionControl::anchorRectangleChanged, this, &DDesktopInputSelectionControl::updateAnchorHandlePosition);
        connect(this, &DDesktopInputSelectionControl::cursorRectangleChanged, this, &DDesktopInputSelectionControl::updateCursorHandlePosition);

        connect(this, &DDesktopInputSelectionControl::anchorRectangleChanged, this, &DDesktopInputSelectionControl::updateTooltipPosition);
        connect(this, &DDesktopInputSelectionControl::cursorRectangleChanged, this, &DDesktopInputSelectionControl::updateTooltipPosition);
    } else {
        disconnect(this, &DDesktopInputSelectionControl::anchorRectangleChanged, this, &DDesktopInputSelectionControl::updateAnchorHandlePosition);
        disconnect(this, &DDesktopInputSelectionControl::cursorRectangleChanged, this, &DDesktopInputSelectionControl::updateCursorHandlePosition);

        disconnect(this, &DDesktopInputSelectionControl::anchorRectangleChanged, this, &DDesktopInputSelectionControl::updateTooltipPosition);
        disconnect(this, &DDesktopInputSelectionControl::cursorRectangleChanged, this, &DDesktopInputSelectionControl::updateTooltipPosition);
    }
}

void DDesktopInputSelectionControl::setHandleState(HandleState state)
{
    m_handleState = state;
}

int DDesktopInputSelectionControl::anchorPosition() const
{
    QInputMethodQueryEvent imQueryEvent(Qt::InputMethodQueries(Qt::ImHints | Qt::ImQueryInput | Qt::ImInputItemClipRectangle));
    const int anchorPosition = imQueryEvent.value(Qt::ImAnchorPosition).toInt();
    return anchorPosition;
}

int DDesktopInputSelectionControl::cursorPosition() const
{
    QInputMethodQueryEvent imQueryEvent(Qt::InputMethodQueries(Qt::ImHints | Qt::ImQueryInput | Qt::ImInputItemClipRectangle));
    const int cursorPosition = imQueryEvent.value(Qt::ImCursorPosition).toInt();
    return cursorPosition;
}

Qt::InputMethodHints DDesktopInputSelectionControl::inputMethodHints() const
{
    QInputMethodQueryEvent imQueryEvent(Qt::InputMethodQueries(Qt::ImHints | Qt::ImQueryInput | Qt::ImInputItemClipRectangle));
    Qt::InputMethodHints inputMethodHints = Qt::InputMethodHints(imQueryEvent.value(Qt::ImHints).toInt());
    return inputMethodHints;
}

QRectF DDesktopInputSelectionControl::anchorRectangle() const
{
    return m_pInputMethod->anchorRectangle();
}

QRectF DDesktopInputSelectionControl::cursorRectangle() const
{
    return m_pInputMethod->cursorRectangle();
}

QRectF DDesktopInputSelectionControl::keyboardRectangle() const
{
    return m_pInputMethod->keyboardRectangle();
}

bool DDesktopInputSelectionControl::anchorRectIntersectsClipRect() const
{
    QInputMethodQueryEvent imQueryEvent(Qt::InputMethodQueries(Qt::ImHints | Qt::ImQueryInput | Qt::ImInputItemClipRectangle));

    QRectF inputItemClipRect = imQueryEvent.value(Qt::ImInputItemClipRectangle).toRectF();
    QRectF anchorRect = imQueryEvent.value(Qt::ImAnchorRectangle).toRectF();

    return inputItemClipRect.intersects(anchorRect);
}

bool DDesktopInputSelectionControl::cursorRectIntersectsClipRect() const
{
    QInputMethodQueryEvent imQueryEvent(Qt::InputMethodQueries(Qt::ImHints | Qt::ImQueryInput | Qt::ImInputItemClipRectangle));

    QRectF inputItemClipRect = imQueryEvent.value(Qt::ImInputItemClipRectangle).toRectF();
    QRectF cursorRect = imQueryEvent.value(Qt::ImCursorRectangle).toRectF();

    return inputItemClipRect.intersects(cursorRect);
}

void DDesktopInputSelectionControl::updateHandleFlags()
{
    if (m_anchorHandleVisible && m_cursorHandleVisible) {
        m_anchorHandleVisible = m_anchorSelectionHandle->isVisible();
        m_cursorHandleVisible = m_cursorSelectionHandle->isVisible();
    }
}

void DDesktopInputSelectionControl::setSelectionOnFocusObject(const QPointF &anchorPos, const QPointF &cursorPos)
{
    return QPlatformInputContext::setSelectionOnFocusObject(anchorPos, cursorPos);
}

void DDesktopInputSelectionControl::setApplicationEventMonitor(DApplicationEventMonitor *pMonitor)
{
    m_pApplicationEventMonitor = pMonitor;
}

bool DDesktopInputSelectionControl::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object)
    QWindow *focusWindow = QGuiApplication::focusWindow();

    if (!m_eventFilterEnabled || focusWindow != object) {
        if (QEvent::FocusOut != event->type()) {
            return false;
        }
    }

    if (!m_focusWindow.isEmpty()) {
        if (!m_anchorSelectionHandle || !m_cursorSelectionHandle || !m_selectedTextTooltip) {
            createHandles();
        }

        if (m_pInputMethod) {
            updateSelectionControlVisible();
        }
    }

    switch (event->type()) {
    case QEvent::FocusOut: {
        if (m_focusWindow.isEmpty())
            break;

        if (auto widget = m_focusWindow.key(m_focusWindow.first())) {
            m_focusWindow.clear();
            widget->removeEventFilter(this);
            m_selectedTextTooltip->hide();
            m_tooptipVisibleFlags = false;
        }
        break;
    }
    case QEvent::ContextMenu: {
        if (m_focusWindow.isEmpty())
            break;
        m_selectedTextTooltip->show();
        updateTooltipPosition();
        m_tooptipVisibleFlags = false;
        return true;
    }
    case QEvent::TouchBegin: {
        QPointF point = anchorRectangle().topLeft();

        if (point.isNull() || m_anchorHandleVisible || m_cursorHandleVisible)
            break;

        QObject *focusWidget = QGuiApplication::focusObject();
        if (m_focusWindow.key(point) == focusWidget) {
            if (m_anchorSelectionHandle && !m_anchorSelectionHandle->isVisible()) {
                m_tooptipVisibleFlags = true;
                if (m_selectedTextTooltip->isVisible()) {
                    m_selectedTextTooltip->hide();
                } else {
                    updateTooltipPosition();
                    m_selectedTextTooltip->show();
                }
            }
        }

        break;
    }
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        const QPoint mousePos = me->screenPos().toPoint();

        // calculate distances from mouse pos to each handle,
        // then choose to interact with the nearest handle
        struct SelectionHandleInfo {
            qreal squaredDistance;
            QPoint delta;
            QRect rect;
        };
        SelectionHandleInfo handles[2];
        handles[AnchorHandle].rect = anchorHandleRect();
        handles[CursorHandle].rect = cursorHandleRect();

        for (int i = 0; i <= CursorHandle; ++i) {
            SelectionHandleInfo &h = handles[i];
            QPoint curHandleCenter = focusWindow->mapToGlobal(h.rect.center()); // ### map to desktoppanel
            const QPoint delta = mousePos - curHandleCenter;
            h.delta = delta;
            h.squaredDistance = QPoint::dotProduct(delta, delta);
        }

        // (squared) distances calculated, pick the closest handle
        HandleType closestHandle = (handles[AnchorHandle].squaredDistance < handles[CursorHandle].squaredDistance ? AnchorHandle : CursorHandle);

        // Can not be replaced with me->windowPos(); because the event might be forwarded from the window of the handle
        const QPoint windowPos = focusWindow->mapFromGlobal(mousePos);
        if (handles[closestHandle].rect.contains(windowPos)) {
            m_currentDragHandle = closestHandle;
            auto currentHandle = closestHandle == CursorHandle ? m_cursorSelectionHandle.data() : m_anchorSelectionHandle.data();

            if (currentHandle->handlePosition() == DInputSelectionHandle::Up) {
                m_distanceBetweenMouseAndCursor = handles[closestHandle].delta - QPoint(0, m_fingerOptSize.height() / 2 + 4);
            } else {
                m_distanceBetweenMouseAndCursor = handles[closestHandle].delta - QPoint(0, -m_fingerOptSize.height() / 2 - 4);
            }

            m_handleState = HandleIsHeld;
            m_handleDragStartedPosition = mousePos;
            const QRect otherRect = handles[1 - closestHandle].rect;
            if (currentHandle->handlePosition() == DInputSelectionHandle::Up) {
                m_otherSelectionPoint = QPoint(otherRect.x() + otherRect.width() / 2, otherRect.top() - 4);
            } else {
                m_otherSelectionPoint = QPoint(otherRect.x() + otherRect.width() / 2, otherRect.bottom() + 4);
            }

            QMouseEvent *mouseEvent = new QMouseEvent(me->type(), me->localPos(), me->windowPos(), me->screenPos(),
                                                      me->button(), me->buttons(), me->modifiers(), me->source());
            m_eventQueue.append(mouseEvent);
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        QPoint mousePos = me->screenPos().toPoint();
        if (m_handleState == HandleIsHeld) {
            QPoint delta = m_handleDragStartedPosition - mousePos;
            const int startDragDistance = QGuiApplication::styleHints()->startDragDistance();
            if (QPoint::dotProduct(delta, delta) > startDragDistance * startDragDistance) {
                m_handleState = HandleIsMoving;
            }
        }
        if (m_handleState == HandleIsMoving) {
            QPoint cursorPos = mousePos - m_distanceBetweenMouseAndCursor;
            cursorPos = focusWindow->mapFromGlobal(cursorPos);
            if (m_currentDragHandle == CursorHandle) {
                setSelectionOnFocusObject(m_otherSelectionPoint, cursorPos);
            } else {
                setSelectionOnFocusObject(cursorPos, m_otherSelectionPoint);
            }

            qDeleteAll(m_eventQueue);
            m_eventQueue.clear();
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        if (m_handleState == HandleIsMoving) {
            m_handleState = HandleIsReleased;
            qDeleteAll(m_eventQueue);
            m_eventQueue.clear();
            return true;
        } else {
            if (QWindow *focusWindow = QGuiApplication::focusWindow()) {
                // playback event queue. These are events that were not designated
                // for the handles in hindsight.
                // This is typically MousePress and MouseRelease (not interleaved with MouseMove)
                // that should instead go through to the underlying input editor
                m_eventFilterEnabled = false;
                while (!m_eventQueue.isEmpty()) {
                    QMouseEvent *e = m_eventQueue.takeFirst();
                    QCoreApplication::sendEvent(focusWindow, e);
                    delete e;
                }
                m_eventFilterEnabled = true;
            }
            m_handleState = HandleIsReleased;
        }
        break;
    }
    default:
        break;
    }

    return false;
}

DPP_END_NAMESPACE
