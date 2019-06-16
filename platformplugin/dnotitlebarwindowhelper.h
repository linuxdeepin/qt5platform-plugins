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
#ifndef DNOTITLEBARWINDOWHELPER_H
#define DNOTITLEBARWINDOWHELPER_H

#include "global.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class DNoTitlebarWindowHelper : public QObject
{
    Q_OBJECT

public:
    explicit DNoTitlebarWindowHelper(QWindow *window);
    ~DNoTitlebarWindowHelper();

    inline QWindow *window() const
    { return reinterpret_cast<QWindow*>(const_cast<DNoTitlebarWindowHelper*>(this));}

    static void setWindowProperty(QWindow *window, const char *name, const QVariant &value);

private slots:
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

private:
    bool windowEvent(QEvent *event);

    bool isEnableSystemMove(quint32 winId);
    static void startMoveWindow(quint32 winId);
    static void updateMoveWindow(quint32 winId);

    bool m_windowMoving = false;

    // properties
    bool m_enableSystemMove = true;
    bool m_autoInputMaskByClipPath = true;

    static QHash<const QWindow*, DNoTitlebarWindowHelper*> mapped;

    friend class DPlatformIntegration;
};

DPP_END_NAMESPACE

#endif // DNOTITLEBARWINDOWHELPER_H
