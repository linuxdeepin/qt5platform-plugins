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
#include "dwaylandinterfacehook.h"

#include <QtWaylandClient/private/qwaylandscreen_p.h>

#include <private/qhighdpiscaling_p.h>
#include <private/qguiapplication_p.h>
#include <QGuiApplication>
#include <QPainter>
#include <QDebug>

DPP_BEGIN_NAMESPACE

bool DHighDpi::active = false;
QDpi DHighDpi::oldDpi = QDpi(0.0, 0.0);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
QHash<QPlatformScreen*, qreal> DHighDpi::screenFactorMap;
#endif
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
            // 可以禁用此行为
            || qEnvironmentVariableIsSet("D_DXCB_DISABLE_OVERRIDE_HIDPI")
            // 无有效的xsettings时禁用
            || !DXcbXSettings::getOwner()
            || (qEnvironmentVariableIsSet("QT_SCALE_FACTOR_ROUNDING_POLICY")
                && qgetenv("QT_SCALE_FACTOR_ROUNDING_POLICY") != "PassThrough")) {
        return;
    }

    // 禁用platform theme中的缩放机制
    qputenv("D_DISABLE_RT_SCREEN_SCALE", "1");
    // 设置为完全控制缩放比例，避免被Qt“4舍5入”了缩放比
    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");
  // 强制开启使用DXCB的缩放机制，此时移除会影响此功能的所有Qt环境变量
    qputenv("D_DXCB_FORCE_OVERRIDE_HIDPI", "1");

    if (qEnvironmentVariableIsSet("D_DXCB_FORCE_OVERRIDE_HIDPI")) {
        qunsetenv("QT_AUTO_SCREEN_SCALE_FACTOR");
        qunsetenv("QT_SCALE_FACTOR");
        qunsetenv("QT_SCREEN_SCALE_FACTORS");
        qunsetenv("QT_ENABLE_HIGHDPI_SCALING");
        qunsetenv("QT_USE_PHYSICAL_DPI");
    }

    if (!QGuiApplication::testAttribute(Qt::AA_EnableHighDpiScaling)) {
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QHighDpiScaling::initHighDpiScaling();
    }

    qDebug()<<QHighDpiScaling::isActive();

    active = VtableHook::overrideVfptrFun(&QtWaylandClient::QWaylandScreen::logicalDpi, logicalDpi);
 }

bool DHighDpi::isActive()
{
    return active;
}

bool DHighDpi::overrideBackingStore()
{
    // 默认不开启，会降低绘图效率
    static bool enabled = qEnvironmentVariableIsSet("D_DXCB_HIDPI_BACKINGSTORE");
    return enabled;
}

QDpi DHighDpi::logicalDpi(QtWaylandClient::QWaylandScreen *s)
{
    static bool isFirstGetDpi = true;

    if (!isFirstGetDpi) {
        return oldDpi;
    }

    static bool dpi_env_set = qEnvironmentVariableIsSet("QT_FONT_DPI");
    // 遵循环境变量的设置
    if (dpi_env_set) {
        return s->QtWaylandClient::QWaylandScreen::logicalDpi();
    }

    int dpi = 0;

    QVariant value = DWaylandInterfaceHook::globalSettings()->setting("Qt/DPI/" + s->name().toLocal8Bit());    bool ok = false;

    dpi = value.toInt(&ok);

    // fallback
    if (!ok) {
        value = DWaylandInterfaceHook::globalSettings()->setting("Xft/DPI");
        dpi = value.toInt(&ok);
    }

    // fallback
    if (!ok) {
        return s->QtWaylandClient::QWaylandScreen::logicalDpi();
    }

    qreal d = dpi / 1024.0;

    if (isFirstGetDpi) {
        isFirstGetDpi = false;
       oldDpi =  QDpi(d, d);
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    // QWaylandScreen 没有实现 pixelDensity，为了不影响兼容性，不能采用 hook pixelDensity 的方式
    QHighDpiScaling::setScreenFactor(s->screen(), d / 96.0);
#endif

    return QDpi(d, d);
}

qreal DHighDpi::devicePixelRatio(QPlatformWindow *w)
{
    qreal base_factor = QHighDpiScaling::factor(w->screen());
    return qCeil(base_factor) / base_factor;
}

void DHighDpi::onDPIChanged(xcb_connection_t *connection, const QByteArray &name, const QVariant &property, void *handle)
{
    static bool dynamic_dpi = qEnvironmentVariableIsSet("D_DXCB_RT_HIDPI");

    if (!dynamic_dpi)
        return;

    Q_UNUSED(connection)
    Q_UNUSED(name)

    // 判断值是否有效
    if (!property.isValid())
        return;

    qDebug() << Q_FUNC_INFO << name << property;

    // 清理过期的屏幕缩放值
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    if (QScreen *screen = reinterpret_cast<QScreen*>(handle)) {
        screenFactorMap.remove(screen->handle());
    } else {
        screenFactorMap.clear();
#else
    {
#endif
        // 刷新所有窗口
    for (QWindow *window : qGuiApp->allWindows()) {
        if (window->type() == Qt::Desktop)
            continue;

        qDebug()<<window->devicePixelRatio();
        // 更新窗口大小
        if (window->handle()) {
            QWindowSystemInterfacePrivate::GeometryChangeEvent gce(window, QHighDpi::fromNativePixels(window->handle()->geometry(), window));
            QGuiApplicationPrivate::processGeometryChangeEvent(&gce);
        }
    }
    }
}

DPP_END_NAMESPACE
