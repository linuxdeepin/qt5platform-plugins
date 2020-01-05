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
#include "dhighdpi.h"
#include "vtablehook.h"
#include "dxcbxsettings.h"
#include "dplatformintegration.h"

#include "qxcbscreen.h"
#include "qxcbwindow.h"

#include <private/qhighdpiscaling_p.h>
#include <private/qguiapplication_p.h>
#include <QGuiApplication>
#include <QDebug>

DPP_BEGIN_NAMESPACE

bool DHighDpi::active = false;
QHash<QPlatformScreen*, qreal> DHighDpi::screenFactorMap;
QPointF DHighDpi::fromNativePixels(const QPointF &pixelPoint, const QWindow *window)
{
    return QHighDpi::fromNativePixels(pixelPoint, window);
}

__attribute__((constructor)) // 在库被加载时就执行此函数
static void init()
{
    DHighDpi::init();
}
void DHighDpi::init()
{
    if (QGuiApplication::testAttribute(Qt::AA_DisableHighDpiScaling)
            || qEnvironmentVariableIsSet("QT_FONT_DPI")
            // 默认不开启此行为
            || qEnvironmentVariableIsEmpty("D_DXCB_OVERRIDE_HIDPI")) {
        return;
    }

    // 禁用platform theme中的缩放机制
    qputenv("D_DISABLE_RT_SCREEN_SCALE", "1");

    if (!QGuiApplication::testAttribute(Qt::AA_EnableHighDpiScaling)) {
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QHighDpiScaling::initHighDpiScaling();
    }

    VtableHook::overrideVfptrFun(&QXcbScreen::logicalDpi, logicalDpi);
    active = VtableHook::overrideVfptrFun(&QXcbScreen::pixelDensity, pixelDensity);
}

bool DHighDpi::isActive()
{
    return active;
}

QDpi DHighDpi::logicalDpi(QXcbScreen *s)
{
    int dpi = 0;
    QVariant value = DPlatformIntegration::xSettings(s->virtualDesktop())->setting("Qt/DPI/" + s->name().toLocal8Bit());
    bool ok = false;

    dpi = value.toInt(&ok);

    // fallback
    if (!ok) {
        value = DPlatformIntegration::xSettings(s->virtualDesktop())->setting("Xft/DPI");
        dpi = value.toInt(&ok);
    }

    // fallback
    if (!ok) {
        return s->QXcbScreen::logicalDpi();
    }

    qreal d = dpi / 1024.0;

    return QDpi(d, d);
}

qreal DHighDpi::pixelDensity(QXcbScreen *s)
{
    qreal scale = screenFactorMap.value(s, 0);

    if (qIsNull(scale)) {
        scale = s->logicalDpi().first / 96.0;
        screenFactorMap[s] = scale;
    }

    return scale;
}

qreal DHighDpi::devicePixelRatio(QPlatformWindow *w)
{
    Q_UNUSED(w)
    return 1.0;
}

void DHighDpi::onDPIChanged(QXcbVirtualDesktop *screen, const QByteArray &name, const QVariant &property, void *handle)
{
    Q_UNUSED(screen)
    Q_UNUSED(name)

    // 判断值是否有效
    if (!property.isValid())
        return;

    qInfo() << Q_FUNC_INFO << name << property;

    // 清理过期的屏幕缩放值
    if (QScreen *screen = reinterpret_cast<QScreen*>(handle)) {
        screenFactorMap.remove(screen->handle());
    } else {
        screenFactorMap.clear();

        // 刷新所有窗口
        for (QWindow *window : qGuiApp->allWindows()) {
            if (window->type() == Qt::Desktop)
                continue;

            // 更新窗口大小
            if (window->handle()) {
                QWindowSystemInterfacePrivate::GeometryChangeEvent gce(window, QHighDpi::fromNativePixels(window->handle()->geometry(), window));
                QGuiApplicationPrivate::processGeometryChangeEvent(&gce);
            }
        }
    }
}

DPP_END_NAMESPACE
