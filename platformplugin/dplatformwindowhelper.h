/*
 * Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
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

#ifndef DPLATFORMWINDOWHELPER_H
#define DPLATFORMWINDOWHELPER_H

#include <QtGlobal>

#ifdef Q_OS_LINUX
#define private public
#include "qxcbwindow.h"
#include "qxcbclipboard.h"
typedef QXcbWindow QNativeWindow;
#undef private
#elif defined(Q_OS_WIN)
#include "qwindowswindow.h"
typedef QWindowsWindow QNativeWindow;
#endif

#include "global.h"
#include "utility.h"

DPP_BEGIN_NAMESPACE

class DFrameWindow;
class DPlatformWindowHelper : public QObject
{
    Q_OBJECT

public:
    explicit DPlatformWindowHelper(QNativeWindow *window);
    ~DPlatformWindowHelper();

    QNativeWindow *window() const
    { return static_cast<QNativeWindow*>(reinterpret_cast<QPlatformWindow*>(const_cast<DPlatformWindowHelper*>(this)));}

    DPlatformWindowHelper *me() const;

    void setGeometry(const QRect &rect);
    QRect geometry() const;
    QRect normalGeometry() const;

    QMargins frameMargins() const;

    void setVisible(bool visible);
    void setWindowFlags(Qt::WindowFlags flags);
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    void setWindowState(Qt::WindowState state);
#else
    void setWindowState(Qt::WindowStates state);
#endif

    WId winId() const;
    void setParent(const QPlatformWindow *window);

    void setWindowTitle(const QString &title);
    void setWindowFilePath(const QString &title);
    void setWindowIcon(const QIcon &icon);
    void raise();
    void lower();

    bool isExposed() const;
//    bool isActive() const;
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    bool isEmbedded() const;
#else
    bool isEmbedded(const QPlatformWindow *parentWindow = 0) const;
#endif

    void propagateSizeHints();

    void setOpacity(qreal level);

    void requestActivateWindow();

    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);

    bool setWindowModified(bool modified);

    bool startSystemResize(const QPoint &pos, Qt::Corner corner);

    void setFrameStrutEventsEnabled(bool enabled);
    bool frameStrutEventsEnabled() const;

    void setAlertState(bool enabled);
    bool isAlertState() const;

private:
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;
    void setNativeWindowGeometry(const QRect &rect, bool onlyResize = false);

    void updateClipPathByWindowRadius(const QSize &windowSize);
    void setClipPath(const QPainterPath &path);
    void setWindowVaildGeometry(const QRect &geometry);
    bool updateWindowBlurAreasForWM();
    void updateSizeHints();
    void updateContentPathForFrameWindow();
    void updateContentWindowGeometry();
#ifdef Q_OS_LINUX
    void updateContentWindowNormalHints();
#endif

    int getWindowRadius() const;
    int getShadowRadius() const;
    QColor getBorderColor() const;

    // update properties
    Q_SLOT void updateClipPathFromProperty();
    Q_SLOT void updateFrameMaskFromProperty();
    Q_SLOT void updateWindowRadiusFromProperty();
    Q_SLOT void updateBorderWidthFromProperty();
    Q_SLOT void updateBorderColorFromProperty();
    Q_SLOT void updateShadowRadiusFromProperty();
    Q_SLOT void updateShadowOffsetFromProperty();
    Q_SLOT void updateShadowColorFromProperty();
    Q_SLOT void updateEnableSystemResizeFromProperty();
    Q_SLOT void updateEnableSystemMoveFromProperty();
    Q_SLOT void updateEnableBlurWindowFromProperty();
    Q_SLOT void updateWindowBlurAreasFromProperty();
    Q_SLOT void updateWindowBlurPathsFromProperty();
    Q_SLOT void updateAutoInputMaskByClipPathFromProperty();

    static void setWindowProperty(QWindow *window, const char *name, const QVariant &value);

    void onFrameWindowContentMarginsHintChanged(const QMargins &old_margins);
    void onWMHasCompositeChanged();

    static QHash<const QPlatformWindow*, DPlatformWindowHelper*> mapped;

    QNativeWindow *m_nativeWindow;
    DFrameWindow *m_frameWindow;

    QRect m_windowVaildGeometry;

    // properties
    bool m_isUserSetClipPath = false;
    QPainterPath m_clipPath;

    bool m_isUserSetFrameMask = false;

    int m_windowRadius = 4;
    bool m_isUserSetWindowRadius = false;

    int m_borderWidth = 1;
    QColor m_borderColor = QColor(0, 0, 0, 255 * 0.15);

    int m_shadowRadius = 60;
    QPoint m_shadowOffset = QPoint(0, 16);
    QColor m_shadowColor = QColor(0, 0, 0, 255 * 0.6);

    bool m_enableSystemResize = true;
    bool m_enableSystemMove = true;
    bool m_enableBlurWindow = false;
    bool m_autoInputMaskByClipPath = true;
    bool m_enableShadow = true;

    QVector<Utility::BlurArea> m_blurAreaList;
    QList<QPainterPath> m_blurPathList;

    friend class DPlatformBackingStoreHelper;
    friend class DPlatformOpenGLContextHelper;
    friend class DPlatformIntegration;
    friend class DPlatformNativeInterfaceHook;
    friend class XcbNativeEventFilter;
    friend class WindowEventHook;
    friend QWindow *topvelWindow(QWindow *);
};

DPP_END_NAMESPACE

#endif // DPLATFORMWINDOWHELPER_H
