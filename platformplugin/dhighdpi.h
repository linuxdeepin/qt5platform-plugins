/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
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
#ifndef DHIGHDPI_H
#define DHIGHDPI_H

#include "global.h"

#include <QPointF>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE
class QWindow;
class QXcbScreen;
class QXcbVirtualDesktop;
class QPlatformWindow;
QT_END_NAMESPACE

DPP_BEGIN_NAMESPACE

class DHighDpi
{
public:
    static QPointF fromNativePixels(const QPointF &pixelPoint, const QWindow *window);

    static void init();
    static bool isActive();
    static QDpi logicalDpi(QXcbScreen *s);
    static qreal pixelDensity(QXcbScreen *s);
    static qreal devicePixelRatio(QPlatformWindow *w);

    static void onDPIChanged(QXcbVirtualDesktop *screen, const QByteArray &name, const QVariant &property, void *handle);

private:

    static bool active;
    static QHash<QPlatformScreen*, qreal> screenFactorMap;
};

DPP_END_NAMESPACE

#endif // DHIGHDPI_H
