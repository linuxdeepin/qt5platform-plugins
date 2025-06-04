// SPDX-FileCopyrightText: 2017 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#define protected public
#include <QWindow>
#undef protected
#include "dnotitlebarwindowhelper.h"
#include "vtablehook.h"
#include "utility.h"
#include "dwmsupport.h"
#include "dnativesettings.h"
#include "dplatformintegration.h"

#include <QMouseEvent>
#include <QTouchEvent>
#include <QTimer>
#include <QMetaProperty>
#include <QScreen>
#include <qpa/qplatformwindow.h>
#include <QGuiApplication>
#include <QStyleHints>

#define _DEEPIN_SCISSOR_WINDOW "_DEEPIN_SCISSOR_WINDOW"
Q_DECLARE_METATYPE(QPainterPath)

extern QWidget *qt_button_down;

DPP_BEGIN_NAMESPACE

QHash<const QWindow*, DNoTitlebarWindowHelper*> DNoTitlebarWindowHelper::mapped;

static QHash<DNoTitlebarWindowHelper*, QPointF> g_pressPoint;

DNoTitlebarWindowHelper::DNoTitlebarWindowHelper(QWindow *window, quint32 windowID)
    : QObject(window)
    , m_window(window)
    , m_windowID(windowID)
{
    // 不允许设置窗口为无边框的
    if (window->flags().testFlag(Qt::FramelessWindowHint))
        window->setFlags(window->flags() & ~Qt::FramelessWindowHint);

    // 初始化窗口类型判断结果
    m_isQuickWindow = window->inherits("QQuickWindow");

    mapped[window] = this;
    m_nativeSettingsValid = DPlatformIntegration::buildNativeSettings(this, windowID);
    Q_ASSERT(m_nativeSettingsValid);

    // 本地设置无效时不可更新窗口属性，否则会导致setProperty函数被循环调用
    if (m_nativeSettingsValid) {
        updateClipPathFromProperty();
        updateFrameMaskFromProperty();
        updateWindowRadiusFromProperty();
        updateBorderWidthFromProperty();
        updateBorderColorFromProperty();
        updateShadowRadiusFromProperty();
        updateShadowOffsetFromProperty();
        updateShadowColorFromProperty();
        updateWindowEffectFromProperty();
        updateWindowStartUpEffectFromProperty();
        updateEnableSystemResizeFromProperty();
        updateEnableSystemMoveFromProperty();
        updateEnableBlurWindowFromProperty();
        updateWindowBlurAreasFromProperty();
        updateWindowBlurPathsFromProperty();
        updateAutoInputMaskByClipPathFromProperty();
    } else {
        qWarning() << "native settings is invalid for window: 0x" << /*hex <<*/ windowID;
    }

    connect(DWMSupport::instance(), &DXcbWMSupport::hasScissorWindowChanged, this, &DNoTitlebarWindowHelper::updateWindowShape);
    connect(DWMSupport::instance(), &DXcbWMSupport::hasBlurWindowChanged, this, [this] (bool blur) {
        // 检测到窗口管理器支持模糊时，应该重新更新模糊属性。否则可能会导致模糊失效，如kwin在某些情况下可能会删除窗口的模糊属性
        if (blur) {
            updateWindowBlurAreasForWM();
        }
    });
    connect(window, &QWindow::widthChanged, this, &DNoTitlebarWindowHelper::onWindowSizeChanged);
    connect(window, &QWindow::heightChanged, this, &DNoTitlebarWindowHelper::onWindowSizeChanged);
}

DNoTitlebarWindowHelper::~DNoTitlebarWindowHelper()
{
    g_pressPoint.remove(this);

    if (VtableHook::hasVtable(m_window)) {
        VtableHook::resetVtable(m_window);
    }

    mapped.remove(static_cast<QWindow*>(parent()));

    if (m_window->handle()) { // 当本地窗口还存在时，移除设置过的窗口属性
        Utility::clearWindowProperty(m_windowID, Utility::internAtom(_DEEPIN_SCISSOR_WINDOW));
        DPlatformIntegration::clearNativeSettings(m_windowID);
    }
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
        // 本地设置无效时不可更新窗口属性，否则会导致setProperty函数被循环调用
        if (!self->m_nativeSettingsValid) {
            return;
        }

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

static QPair<qreal, qreal> takePair(const QVariant &value, const QPair<qreal, qreal> defaultValue)
{
    if (!value.isValid()) {
        return defaultValue;
    }

    const QStringList &l = value.toString().split(',');

    if (l.count() < 2) {
        return defaultValue;
    }

    QPair<qreal, qreal> ret;

    ret.first = l.first().toDouble();
    ret.second = l.at(1).toDouble();

    return ret;
}

static QMarginsF takeMargins(const QVariant &value, const QMarginsF &defaultValue)
{
    if (!value.isValid()) {
        return defaultValue;
    }

    const QStringList &l = value.toString().split(',');

    if (l.count() < 4) {
        return defaultValue;
    }

    return QMarginsF(l.at(0).toDouble(), l.at(1).toDouble(),
                     l.at(2).toDouble(), l.at(3).toDouble());
}

static inline QPointF toPos(const QPair<qreal, qreal> &pair)
{
    return QPointF(pair.first, pair.second);
}

QString DNoTitlebarWindowHelper::theme() const
{
    return property("theme").toString();
}

QPointF DNoTitlebarWindowHelper::windowRadius() const
{
    return toPos(takePair(property("windowRadius"), qMakePair(0.0, 0.0)));
}

qreal DNoTitlebarWindowHelper::borderWidth() const
{
    return property("borderWidth").toDouble();
}

QColor DNoTitlebarWindowHelper::borderColor() const
{
    return qvariant_cast<QColor>(property("borderColor"));
}

qreal DNoTitlebarWindowHelper::shadowRadius() const
{
    return property("shadowRadius").toDouble();
}

QPointF DNoTitlebarWindowHelper::shadowOffset() const
{
    return toPos(takePair(property("shadowOffset"), qMakePair(0.0, 0.0)));
}

QColor DNoTitlebarWindowHelper::shadowColor() const
{
    return qvariant_cast<QColor>(property("shadowColor"));
}

quint32 DNoTitlebarWindowHelper::windowEffect()
{
    return qvariant_cast<quint32>(property("windowEffect"));
}

quint32 DNoTitlebarWindowHelper::windowStartUpEffect()
{
    return qvariant_cast<quint32>(property("windowStartUpEffect"));
}

QMarginsF DNoTitlebarWindowHelper::mouseInputAreaMargins() const
{
    return takeMargins(property("mouseInputAreaMargins"), QMarginsF(0, 0, 0, 0));
}

void DNoTitlebarWindowHelper::resetProperty(const QByteArray &property)
{
    int index = metaObject()->indexOfProperty(property.constData());

    if (index >= 0) {
        metaObject()->property(index).reset(this);
    }
}

void DNoTitlebarWindowHelper::setTheme(const QString &theme)
{
    setProperty("theme", theme);
}

void DNoTitlebarWindowHelper::setWindowRadius(const QPointF &windowRadius)
{
    setProperty("windowRadius", QString("%1,%2").arg(windowRadius.x()).arg(windowRadius.y()));
}

void DNoTitlebarWindowHelper::setBorderWidth(qreal borderWidth)
{
    setProperty("borderWidth", borderWidth);
}

void DNoTitlebarWindowHelper::setBorderColor(const QColor &borderColor)
{
    setProperty("borderColor", QVariant::fromValue(borderColor));
}

void DNoTitlebarWindowHelper::setShadowRadius(qreal shadowRadius)
{
    setProperty("shadowRadius", shadowRadius);
}

void DNoTitlebarWindowHelper::setShadowOffect(const QPointF &shadowOffect)
{
    setProperty("shadowOffset", QString("%1,%2").arg(shadowOffect.x()).arg(shadowOffect.y()));
}

void DNoTitlebarWindowHelper::setShadowColor(const QColor &shadowColor)
{
    setProperty("shadowColor", QVariant::fromValue(shadowColor));
}

void DNoTitlebarWindowHelper::setMouseInputAreaMargins(const QMarginsF &mouseInputAreaMargins)
{
    setProperty("mouseInputAreaMargins", QString("%1,%2,%3,%4").arg(mouseInputAreaMargins.left()).arg(mouseInputAreaMargins.top())
                                                               .arg(mouseInputAreaMargins.right()).arg(mouseInputAreaMargins.bottom()));
}

void DNoTitlebarWindowHelper::setWindowEffect(quint32 effectScene)
{
    setProperty("windowEffect", effectScene);
}

void DNoTitlebarWindowHelper::setWindowStartUpEffect(quint32 effectType)
{
    setProperty("windowStartUpEffect", effectType);
}

DNoTitlebarWindowHelper *DNoTitlebarWindowHelper::windowHelper(const QWindow *window)
{
    return mapped.value(window);
}

void DNoTitlebarWindowHelper::updateClipPathFromProperty()
{
    const QVariant &v = m_window->property(clipPath);
    const QPainterPath &path = qvariant_cast<QPainterPath>(v);
    static xcb_atom_t _deepin_scissor_window = Utility::internAtom(_DEEPIN_SCISSOR_WINDOW, false);

    if (!path.isEmpty()) {
        m_clipPath = path * m_window->screen()->devicePixelRatio();

        QByteArray data;
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << m_clipPath;
        Utility::setWindowProperty(m_windowID, _deepin_scissor_window, _deepin_scissor_window, data.constData(), data.length(), 8);
    } else {
        m_clipPath = QPainterPath();
        Utility::clearWindowProperty(m_windowID, _deepin_scissor_window);
    }

    updateWindowShape();
}

void DNoTitlebarWindowHelper::updateFrameMaskFromProperty()
{
    // TODO
}

void DNoTitlebarWindowHelper::updateWindowRadiusFromProperty()
{
    const QVariant &v = m_window->property("_d_windowRadius");
    bool ok;
    int radius = v.toInt(&ok);

    if (ok) {
        setWindowRadius(QPointF(radius, radius) * m_window->screen()->devicePixelRatio());
    } else {
        resetProperty("windowRadius");
    }
}

void DNoTitlebarWindowHelper::updateBorderWidthFromProperty()
{
    const QVariant &v = m_window->property("_d_borderWidth");
    bool ok;
    int width = v.toInt(&ok);

    if (ok) {
        setBorderWidth(width);
    } else {
        resetProperty("borderWidth");
    }
}

void DNoTitlebarWindowHelper::updateBorderColorFromProperty()
{
    const QVariant &v = m_window->property("_d_borderColor");
    const QColor &color = qvariant_cast<QColor>(v);

    if (color.isValid()) {
        setBorderColor(color);
    } else {
        resetProperty("borderColor");
    }
}

void DNoTitlebarWindowHelper::updateShadowRadiusFromProperty()
{
    const QVariant &v = m_window->property("_d_shadowRadius");
    bool ok;
    int radius = v.toInt(&ok);

    if (ok) {
        setShadowRadius(radius);
    } else {
        resetProperty("shadowRadius");
    }
}

void DNoTitlebarWindowHelper::updateShadowOffsetFromProperty()
{
    const QVariant &pos = m_window->property("_d_shadowOffset");

    if (pos.isValid()) {
        setShadowOffect(pos.toPoint());
    } else {
        resetProperty("shadowOffset");
    }
}

void DNoTitlebarWindowHelper::updateShadowColorFromProperty()
{
    const QVariant &v = m_window->property("_d_shadowColor");
    const QColor &color = qvariant_cast<QColor>(v);

    if (color.isValid()) {
        setShadowColor(color);
    } else {
        resetProperty("shadowColor");
    }
}

void DNoTitlebarWindowHelper::updateWindowEffectFromProperty()
{
    const QVariant &v = m_window->property("_d_windowEffect");
    const quint32 effectScene = qvariant_cast<quint32>(v);

    if (effectScene) {
        setWindowEffect(effectScene);
    } else {
        resetProperty("windowEffect");
    }
}

void DNoTitlebarWindowHelper::updateWindowStartUpEffectFromProperty()
{
    const QVariant &v = m_window->property("_d_windowStartUpEffect");
    const quint32 effectType = qvariant_cast<quint32>(v);

    if (effectType) {
        setWindowStartUpEffect(effectType);
    } else {
        resetProperty("windowStartUpEffect");
    }
}

void DNoTitlebarWindowHelper::updateEnableSystemResizeFromProperty()
{
    const QVariant &v = m_window->property(enableSystemResize);

    if (v.isValid() && !v.toBool()) {
        setMouseInputAreaMargins(QMarginsF(0, 0, 0, 0));
    } else {
        resetProperty("mouseInputAreaMargins");
    }
}

void DNoTitlebarWindowHelper::updateEnableSystemMoveFromProperty()
{
    const QVariant &v = m_window->property(enableSystemMove);

    m_enableSystemMove = !v.isValid() || v.toBool();

    if (m_enableSystemMove) {
        VtableHook::overrideVfptrFun(m_window, &QWindow::event, this, &DNoTitlebarWindowHelper::windowEvent);
    } else if (VtableHook::hasVtable(m_window)) {
        VtableHook::resetVfptrFun(m_window, &QWindow::event);
    }
}

void DNoTitlebarWindowHelper::updateEnableBlurWindowFromProperty()
{
    const QVariant &v = m_window->property(enableBlurWindow);

    if (!v.isValid()) {
        m_window->setProperty(enableBlurWindow, m_enableBlurWindow);

        return;
    }

    if (m_enableBlurWindow != v.toBool()) {
        m_enableBlurWindow = v.toBool();

        if (m_enableBlurWindow) {
            connect(DWMSupport::instance(), &DWMSupport::windowManagerChanged,
                    this, &DNoTitlebarWindowHelper::updateWindowBlurAreasForWM);
        } else {
            disconnect(DWMSupport::instance(), &DWMSupport::windowManagerChanged,
                       this, &DNoTitlebarWindowHelper::updateWindowBlurAreasForWM);
        }

        updateWindowBlurAreasForWM();
    }
}

void DNoTitlebarWindowHelper::updateWindowBlurAreasFromProperty()
{
    const QVariant &v = m_window->property(windowBlurAreas);
    const QVector<quint32> &tmpV = qvariant_cast<QVector<quint32>>(v);
    const int OffSet = 6; // Utility::BlurArea's member variables

    Q_ASSERT(tmpV.size() % OffSet == 0);

    QVector<Utility::BlurArea> a;
    for (int i = 0; i < tmpV.size(); i += OffSet) {
        Utility::BlurArea area;
        area.x = tmpV[i + 0];
        area.y = tmpV[i + 1];
        area.width = tmpV[i + 2];
        area.height = tmpV[i + 3];
        area.xRadius = tmpV[i + 4];
        area.yRaduis = tmpV[i + 5];
        a << area;
    }

    if (a.isEmpty() && m_blurAreaList.isEmpty())
        return;

    m_blurAreaList = a;

    updateWindowBlurAreasForWM();
}

void DNoTitlebarWindowHelper::updateWindowBlurPathsFromProperty()
{
    const QVariant &v = m_window->property(windowBlurPaths);
    const QList<QPainterPath> paths = qvariant_cast<QList<QPainterPath>>(v);

    if (paths.isEmpty() && m_blurPathList.isEmpty())
        return;

    m_blurPathList = paths;

    updateWindowBlurAreasForWM();
}

void DNoTitlebarWindowHelper::updateAutoInputMaskByClipPathFromProperty()
{
    bool auto_update_shape = m_window->property(autoInputMaskByClipPath).toBool();

    if (auto_update_shape == m_autoInputMaskByClipPath)
        return;

    m_autoInputMaskByClipPath = auto_update_shape;

    updateWindowShape();
}

bool DNoTitlebarWindowHelper::windowEvent(QEvent *event)
{
    QWindow *w = this->window();

    // TODO Crashed when delete by Vtable.
    if (event->type() == QEvent::DeferredDelete) {
        VtableHook::resetVtable(w);
        return w->event(event);
    }
    DNoTitlebarWindowHelper *self = mapped.value(w);

    // get touch begin position
    static bool isTouchDown = false;
    static QPointF touchBeginPosition;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    static QHash<int, QObject*> touchPointGrabbers; // 新增：记录触摸点的grabber状态
    const auto isQuickWindow = self->m_isQuickWindow;
#endif

    if (event->type() == QEvent::TouchBegin) {
        isTouchDown = true;
        // ========== 新增：记录处理前的grabber状态 ==========
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (isQuickWindow && event->isPointerEvent()) {
            QPointerEvent *pe = static_cast<QPointerEvent*>(event);
            touchPointGrabbers.clear();
            for (const auto &point : pe->points()) {
                QObject *grabber = pe->exclusiveGrabber(point);
                touchPointGrabbers[point.id()] = grabber;
            }
        }
#endif
    }
    if (event->type() == QEvent::TouchEnd || event->type() == QEvent::MouseButtonRelease) {
        isTouchDown = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        touchPointGrabbers.clear(); // 清理grabber记录
#endif
    }
    if (isTouchDown && event->type() == QEvent::MouseButtonPress) {
        touchBeginPosition = static_cast<QMouseEvent*>(event)->globalPos();
    }
    // add some redundancy to distinguish trigger between system menu and system move
    if (event->type() == QEvent::MouseMove) {
        QPointF currentPos = static_cast<QMouseEvent*>(event)->globalPos();
        QPointF delta = touchBeginPosition  - currentPos;
        if (delta.manhattanLength() < QGuiApplication::styleHints()->startDragDistance()) {
            return VtableHook::callOriginalFun(w, &QWindow::event, event);
        }
    }

    quint32 winId = self->m_windowID;
    bool is_mouse_move = event->type() == QEvent::MouseMove && static_cast<QMouseEvent*>(event)->buttons() == Qt::LeftButton;

    if (event->type() == QEvent::MouseButtonRelease) {
        self->m_windowMoving = false;
        Utility::updateMousePointForWindowMove(winId, true);
        g_pressPoint.remove(this);
    }

    if (is_mouse_move && self->m_windowMoving) {
        updateMoveWindow(winId);
    }

    bool ret = VtableHook::callOriginalFun(w, &QWindow::event, event);

    // workaround for kwin: Qt receives no release event when kwin finishes MOVE operation,
    // which makes app hang in windowMoving state. when a press happens, there's no sense of
    // keeping the moving state, we can just reset ti back to normal.
    if (event->type() == QEvent::MouseButtonPress) {
        self->m_windowMoving = false;
        g_pressPoint[this] = dynamic_cast<QMouseEvent*>(event)->globalPos();
    }

    // ========== 修改：鼠标事件处理保持原有逻辑 ==========
    if (is_mouse_move && !event->isAccepted() && g_pressPoint.contains(this)) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QRect windowRect = QRect(QPoint(0, 0), w->size());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (!windowRect.contains(me->scenePosition().toPoint())) {
#else
        if (!windowRect.contains(me->localPos().toPoint())) {
#endif
            return ret;
        }

        QPointF delta = me->globalPos() - g_pressPoint[this];
        if (delta.manhattanLength() < QGuiApplication::styleHints()->startDragDistance()) {
            return ret;
        }

        if (!self->m_windowMoving && self->isEnableSystemMove(winId)) {
            qDebug() << "Starting mouse drag for window ID:" << winId;
            self->m_windowMoving = true;

            event->accept();
            startMoveWindow(winId);

            if (qt_button_down) {
                qt_button_down = nullptr;
            }
        }
    }

    // ========== 新增：QML窗口触摸事件的特殊处理 ==========
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (isQuickWindow && event->isPointerEvent()) {
        QPointerEvent *pe = static_cast<QPointerEvent*>(event);
        
        // 检查是否是触摸事件
        bool isTouchEvent = (event->type() == QEvent::TouchBegin || 
                            event->type() == QEvent::TouchUpdate || 
                            event->type() == QEvent::TouchEnd);
        
        if (isTouchEvent) {
            // 检查是否有新的grabber产生（说明QML Item处理了触摸事件）
            bool wasHandledByQML = false;
            for (const auto &point : pe->points()) {
                QObject *grabberBefore = touchPointGrabbers.value(point.id());
                QObject *grabberAfter = pe->exclusiveGrabber(point);
                
                if (grabberBefore != grabberAfter && grabberAfter != nullptr) {
                    wasHandledByQML = true;
                    break;
                }
            }
            
            // 如果没有QML Item处理触摸事件，尝试窗口拖拽
            if (!wasHandledByQML && self->isEnableSystemMove(winId)) {
                if (self->handleTouchDragForQML(pe)) {
                    return ret;
                }
            }
        }
    }
#endif

    return ret;
}

bool DNoTitlebarWindowHelper::isEnableSystemMove(quint32 winId)
{
    if (!m_enableSystemMove)
        return false;

#ifdef Q_OS_LINUX
    quint32 hints = DXcbWMSupport::getMWMFunctions(Utility::getNativeTopLevelWindow(winId));

    return ((hints & DXcbWMSupport::MWM_FUNC_ALL) || hints & DXcbWMSupport::MWM_FUNC_MOVE);
#endif

    return true;
}

bool DNoTitlebarWindowHelper::updateWindowBlurAreasForWM()
{
    bool blurEnable = m_enableBlurWindow || !m_blurAreaList.isEmpty() || !m_blurPathList.isEmpty();
    // NOTE(zccrs): 当窗口unmap时清理模糊区域的属性，否则可能会导致窗口已经隐藏，但模糊区域没有消失的问题。因为窗口管理器不是绝对根据窗口是否map来绘制
    //              模糊背景，当窗口unmap但是窗口的WM State值为Normal时也会绘制模糊背景（这种情况可能出现在连续多次快速切换窗口管理器时）
    if (!blurEnable || !DWMSupport::instance()->hasBlurWindow()) {
        Utility::clearWindowBlur(m_windowID);

        return true;
    }

    qreal device_pixel_ratio = m_window->screen()->devicePixelRatio();
    quint32 top_level_w = Utility::getNativeTopLevelWindow(m_windowID);
    QPoint offset = QPoint(0, 0);
    const bool is_toplevel_window = (top_level_w == m_windowID);

    if (!is_toplevel_window) {
        offset += Utility::translateCoordinates(QPoint(0, 0), m_windowID, top_level_w);
    }

    QVector<Utility::BlurArea> newAreas;

    if (m_enableBlurWindow) {
        m_needUpdateBlurAreaForWindowSizeChanged = !is_toplevel_window || !Utility::setEnableBlurWindow(m_windowID, true);

        if (m_needUpdateBlurAreaForWindowSizeChanged) {
            const QRect &windowValidRect = QRect(QPoint(0, 0), m_window->size() * device_pixel_ratio);
            Utility::BlurArea area;

            area.x = windowValidRect.x() + offset.x();
            area.y = windowValidRect.y() + offset.y();
            area.width = windowValidRect.width();
            area.height = windowValidRect.height();
            area.xRadius = 0;
            area.yRaduis = 0;

            newAreas.append(std::move(area));

            return Utility::blurWindowBackground(top_level_w, newAreas);
        } else {
            return true;
        }
    }

    if (m_blurPathList.isEmpty()) {
        if (m_blurAreaList.isEmpty())
            return true;

        newAreas.reserve(m_blurAreaList.size());

        foreach (Utility::BlurArea area, m_blurAreaList) {
            area *= device_pixel_ratio;

            area.x += offset.x();
            area.y += offset.y();

            newAreas.append(std::move(area));
        }

        if (newAreas.size() == m_blurAreaList.size())
            return Utility::blurWindowBackground(top_level_w, newAreas);
    }

    QList<QPainterPath> newPathList;

    newPathList.reserve(m_blurAreaList.size());

    foreach (Utility::BlurArea area, m_blurAreaList) {
        QPainterPath path;

        area *= device_pixel_ratio;
        path.addRoundedRect(area.x + offset.x(), area.y + offset.y(), area.width, area.height,
                            area.xRadius, area.yRaduis);

        if (!path.isEmpty())
            newPathList << path;
    }

    if (!m_blurPathList.isEmpty()) {
        newPathList.reserve(newPathList.size() + m_blurPathList.size());

        foreach (const QPainterPath &path, m_blurPathList) {
            newPathList << (path * device_pixel_ratio).translated(offset);
        }
    }

    if (newPathList.isEmpty())
        return true;

    return Utility::blurWindowBackgroundByPaths(top_level_w, newPathList);
}

void DNoTitlebarWindowHelper::updateWindowShape()
{
    if (m_clipPath.isEmpty()) {
        return Utility::setShapePath(m_windowID, m_clipPath, false, false);
    }

    if (DWMSupport::instance()->hasScissorWindow()) {
        Utility::setShapePath(m_windowID, m_clipPath, true, m_autoInputMaskByClipPath);
    } else {
        Utility::setShapePath(m_windowID, m_clipPath, false, false);
    }
}

void DNoTitlebarWindowHelper::onWindowSizeChanged()
{
    if (m_needUpdateBlurAreaForWindowSizeChanged) {
        updateWindowBlurAreasForWM();
    }
}

void DNoTitlebarWindowHelper::startMoveWindow(quint32 winId)
{
    Utility::startWindowSystemMove(winId);
}

void DNoTitlebarWindowHelper::updateMoveWindow(quint32 winId)
{
    Utility::updateMousePointForWindowMove(winId);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool DNoTitlebarWindowHelper::handleTouchDragForQML(QPointerEvent *touchEvent)
{
    QWindow *w = this->m_window;
    static QHash<int, QPointF> touchStartPositions;
    static QHash<int, bool> touchIsMoving;
    static QHash<int, QPointF> lastTouchPositions; // 新增：记录最后的触摸位置用于更新
    
    const auto points = touchEvent->points();
    if (points.isEmpty()) {
        return false;
    }
    
    // 只处理第一个触摸点
    const auto &point = points.first();
    int touchId = point.id();
    
    // ========== 新增：多屏幕坐标处理 ==========
    QScreen *windowScreen = w->screen();
    QPointF globalPos = point.globalPosition();
    QPointF localPos = point.position();
    
    switch (touchEvent->type()) {
    case QEvent::TouchBegin: {
        touchStartPositions[touchId] = globalPos;
        lastTouchPositions[touchId] = globalPos;
        touchIsMoving[touchId] = false;
        return false; // 不消费事件，让QML先处理
    }
    
    case QEvent::TouchUpdate: {
        if (!touchStartPositions.contains(touchId)) {
            return false;
        }
        
        QPointF startPos = touchStartPositions[touchId];
        QPointF currentPos = globalPos;
        QPointF delta = currentPos - startPos;
        qreal distance = delta.manhattanLength();
        qreal threshold = QGuiApplication::styleHints()->startDragDistance();
        
        // 更新最后触摸位置
        lastTouchPositions[touchId] = currentPos;
        
        // 检查移动距离是否达到拖拽阈值
        if (!touchIsMoving[touchId] && distance >= threshold) {
            
            // ========== 修改：多屏幕环境下的窗口边界检查 ==========
            QRect windowRect = QRect(QPoint(0, 0), w->size());
            QPoint localTouchPos = localPos.toPoint();
            bool inBounds = windowRect.contains(localTouchPos);
            
            // 使用本地坐标检查（更可靠）
            if (!inBounds) {
                return false;
            }
            
            // 开始窗口移动
            if (isEnableSystemMove(m_windowID)) {
                qDebug() << "Starting touch drag for QML window ID:" << m_windowID
                         << "on screen:" << (windowScreen ? windowScreen->name() : "unknown");
                m_windowMoving = true;
                touchIsMoving[touchId] = true;
                
                startMoveWindow(m_windowID);
                touchEvent->accept();
                return true;
            }
        } else if (touchIsMoving[touchId] && m_windowMoving) {
            // ========== 修改：使用带坐标参数的updateMousePointForWindowMove ==========
            Utility::updateMousePointForWindowMove(m_windowID, currentPos.toPoint());
            touchEvent->accept();
            return true;
        }
        break;
    }
    
    case QEvent::TouchEnd: {
        if (touchIsMoving.value(touchId, false)) {
            m_windowMoving = false;
            // ========== 修改：使用最后的触摸位置结束窗口移动 ==========
            QPointF lastPos = lastTouchPositions.value(touchId, globalPos);
            Utility::updateMousePointForWindowMove(m_windowID, lastPos.toPoint(), true);
            qDebug() << "Finished touch drag for QML window ID:" << m_windowID;
        }
        
        // 清理状态
        touchStartPositions.remove(touchId);
        touchIsMoving.remove(touchId);
        lastTouchPositions.remove(touchId);
        break;
    }
    
    default:
        break;
    }
    
    return false;
}
#endif

DPP_END_NAMESPACE
