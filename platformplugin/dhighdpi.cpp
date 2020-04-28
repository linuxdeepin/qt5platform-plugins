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
#include "qxcbbackingstore.h"

#include <private/qhighdpiscaling_p.h>
#include <private/qguiapplication_p.h>
#include <QGuiApplication>
#include <QDebug>

DPP_BEGIN_NAMESPACE

bool DHighDpi::active = false;
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

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    active = VtableHook::overrideVfptrFun(&QXcbScreen::pixelDensity, pixelDensity);

    if (active) {
        VtableHook::overrideVfptrFun(&QXcbScreen::logicalDpi, logicalDpi);
    }
#else
    active = VtableHook::overrideVfptrFun(&QXcbScreen::logicalDpi, logicalDpi);
#endif
}

bool DHighDpi::isActive()
{
    return active;
}

bool DHighDpi::overrideBackingStore()
{
    // 默认不开启，会降低绘图效率
    static bool enabled = qEnvironmentVariableIsSet("D_DXCB_HIDPI_BACKINGSTOR");
    return enabled;
}

QDpi DHighDpi::logicalDpi(QXcbScreen *s)
{
    static bool dpi_env_set = qEnvironmentVariableIsSet("QT_FONT_DPI");
    // 遵循环境变量的设置
    if (dpi_env_set) {
        return s->QXcbScreen::logicalDpi();
    }

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

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
qreal DHighDpi::pixelDensity(QXcbScreen *s)
{
    qreal scale = screenFactorMap.value(s, 0);

    if (qIsNull(scale)) {
        scale = s->logicalDpi().first / 96.0;
        screenFactorMap[s] = scale;
    }

    return scale;
}
#endif

qreal DHighDpi::devicePixelRatio(QPlatformWindow *w)
{
    qreal base_factor = QHighDpiScaling::factor(w->screen());
    return qCeil(base_factor) / base_factor;
}

void DHighDpi::onDPIChanged(QXcbVirtualDesktop *screen, const QByteArray &name, const QVariant &property, void *handle)
{
    static bool dynamic_dpi = qEnvironmentVariableIsSet("D_DXCB_RT_HIDPI");

    if (!dynamic_dpi)
        return;

    Q_UNUSED(screen)
    Q_UNUSED(name)

    // 判断值是否有效
    if (!property.isValid())
        return;

    qInfo() << Q_FUNC_INFO << name << property;

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

            // 更新窗口大小
            if (window->handle()) {
                QWindowSystemInterfacePrivate::GeometryChangeEvent gce(window, QHighDpi::fromNativePixels(window->handle()->geometry(), window));
                QGuiApplicationPrivate::processGeometryChangeEvent(&gce);
            }
        }
    }
}

DHighDpi::BackingStore::BackingStore(QPlatformBackingStore *proxy)
    : QPlatformBackingStore(proxy->window())
    , m_proxy(proxy)
{

}

DHighDpi::BackingStore::~BackingStore()
{
    delete m_proxy;
}

QPaintDevice *DHighDpi::BackingStore::paintDevice()
{
    return !m_image.isNull()? &m_image : m_proxy->paintDevice();
}

void DHighDpi::BackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (!m_image.isNull()) {
        QRegion expand_region;

        for (const QRect &r : region) {
            expand_region += r.adjusted(-1, -1, 1, 1);
        }

        m_proxy->flush(window, expand_region, offset);
    } else { // 未开启缩放补偿
        m_proxy->flush(window, region, offset);
    }
}

#ifndef QT_NO_OPENGL
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
void DHighDpi::BackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                             QPlatformTextureList *textures, QOpenGLContext *context)
{
    m_proxy->composeAndFlush(window, region, offset, textures, context);
}
#elif QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
void DHighDpi::BackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                             QPlatformTextureList *textures, QOpenGLContext *context,
                                             bool translucentBackground)
{
    m_proxy->composeAndFlush(window, region, offset, textures, context, translucentBackground);
}
#else
void DHighDpi::BackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                             QPlatformTextureList *textures,
                                             bool translucentBackground)
{
    m_proxy->composeAndFlush(window, region, offset, textures, translucentBackground);
}
#endif
GLuint DHighDpi::BackingStore::toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const
{
    return m_proxy->toTexture(dirtyRegion, textureSize, flags);
}
#endif

QImage DHighDpi::BackingStore::toImage() const
{
    return m_proxy->toImage();
}

QPlatformGraphicsBuffer *DHighDpi::BackingStore::graphicsBuffer() const
{
    return m_proxy->graphicsBuffer();
}

void DHighDpi::BackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    m_proxy->resize(size, staticContents);

    if (!QHighDpiScaling::isActive()) {
        // 清理hidpi缓存
        m_image = QImage();
        return;
    }

    qreal scale = QHighDpiScaling::factor(window());
    // 小数倍缩放时才开启此功能
    if (qCeil(scale) != qFloor(scale)) {
        bool hasAlpha = toImage().pixelFormat().alphaUsage() == QPixelFormat::UsesAlpha;
        m_image = QImage(window()->size() * window()->devicePixelRatio(),
                         hasAlpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
    }
}

bool DHighDpi::BackingStore::scroll(const QRegion &area, int dx, int dy)
{
    return m_proxy->scroll(area, dx, dy);
}

void DHighDpi::BackingStore::beginPaint(const QRegion &region)
{
    m_proxy->beginPaint(region);

    if (m_image.isNull()) {
        return;
    }

    qreal window_scale = window()->devicePixelRatio();
    m_dirtyRect = QRect();

    QPainter p(&m_image);
    p.setCompositionMode(QPainter::CompositionMode_Clear);

    for (QRect rect : region) {
        rect = QHighDpi::fromNativePixels(rect, window());
        rect = QRect(rect.topLeft() * window_scale, QHighDpi::toNative(rect.size(), window_scale));

        // 如果是透明绘制，应当先清理要绘制的区域
        if (m_image.format() == QImage::Format_ARGB32_Premultiplied)
            p.fillRect(rect, Qt::transparent);

        m_dirtyRect |= rect;
    }

    p.end();

    if (m_dirtyRect.isValid()) {
        // 额外扩大一个像素，用于补贴两个不同尺寸图片上的绘图误差
        m_dirtyRect.adjust(-window_scale, -window_scale,
                           window_scale, window_scale);

        m_dirtyWindowRect = QRectF(m_dirtyRect.topLeft() / window_scale,
                                   m_dirtyRect.size() / window_scale);
        m_dirtyWindowRect = QHighDpi::toNativePixels(m_dirtyWindowRect, window());
    } else {
        m_dirtyWindowRect = QRectF();
    }
}

void DHighDpi::BackingStore::endPaint()
{
    QPainter pa(m_proxy->paintDevice());
    pa.setRenderHints(QPainter::SmoothPixmapTransform);
    pa.setCompositionMode(QPainter::CompositionMode_Source);
    pa.drawImage(m_dirtyWindowRect, m_image, m_dirtyRect);
    pa.end();

    m_proxy->endPaint();
}

DPP_END_NAMESPACE
