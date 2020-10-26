/*
 * Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
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
#include "dbackingstoreproxy.h"
#include "dopenglpaintdevice.h"

#include <QGuiApplication>
#include <QDebug>
#include <QPainter>
#include <QOpenGLPaintDevice>

#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>

DPP_BEGIN_NAMESPACE

bool DBackingStoreProxy::useGLPaint(const QWindow *w)
{
#ifndef QT_NO_OPENDL
    if (!w->supportsOpenGL())
        return false;

    if (qEnvironmentVariableIsSet("D_NO_OPENGL") || qEnvironmentVariableIsSet("D_NO_HARDWARE_ACCELERATION"))
        return false;

    bool envIsIntValue = false;
    bool forceGLPaint = qEnvironmentVariableIntValue("D_USE_GL_PAINT", &envIsIntValue) == 1;
    QVariant value = w->property(enableGLPaint);

    if (envIsIntValue && !forceGLPaint) {
        return false;
    }

    return value.isValid() ? value.toBool() : forceGLPaint;
#else
    return false;
#endif
}

DBackingStoreProxy::DBackingStoreProxy(QPlatformBackingStore *proxy, bool useGLPaint)
    : QPlatformBackingStore(proxy->window())
    , m_proxy(proxy)
    , enableGL(useGLPaint)
{

}

DBackingStoreProxy::~DBackingStoreProxy()
{
    delete m_proxy;
}

QPaintDevice *DBackingStoreProxy::paintDevice()
{
    if (glDevice)
        return glDevice.data();

    return !m_image.isNull()? &m_image : m_proxy->paintDevice();
}

void DBackingStoreProxy::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (glDevice)
        return glDevice->flush();

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
void DOpenGLBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                             QPlatformTextureList *textures, QOpenGLContext *context)
{
    m_proxy->composeAndFlush(window, region, offset, textures, context);
}
#elif QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
void DOpenGLBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                             QPlatformTextureList *textures, QOpenGLContext *context,
                                             bool translucentBackground)
{
    m_proxy->composeAndFlush(window, region, offset, textures, context, translucentBackground);
}
#else
void DBackingStoreProxy::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                             QPlatformTextureList *textures,
                                             bool translucentBackground)
{
    m_proxy->composeAndFlush(window, region, offset, textures, translucentBackground);
}
#endif
GLuint DBackingStoreProxy::toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const
{
    return m_proxy->toTexture(dirtyRegion, textureSize, flags);
}
#endif

QImage DBackingStoreProxy::toImage() const
{
    return m_proxy->toImage();
}

QPlatformGraphicsBuffer *DBackingStoreProxy::graphicsBuffer() const
{
    return m_proxy->graphicsBuffer();
}

void DBackingStoreProxy::resize(const QSize &size, const QRegion &staticContents)
{
    if (Q_LIKELY(enableGL)) {
        if (Q_LIKELY(glDevice)) {
            glDevice->resize(size);
        } else {
            glDevice.reset(new DOpenGLPaintDevice(window(), DOpenGLPaintDevice::PartialUpdateBlit));
        }

        return;
    }

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

bool DBackingStoreProxy::scroll(const QRegion &area, int dx, int dy)
{
    return m_proxy->scroll(area, dx, dy);
}

void DBackingStoreProxy::beginPaint(const QRegion &region)
{
    if (glDevice)
        return;

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

void DBackingStoreProxy::endPaint()
{
    if (glDevice)
        return;

    QPainter pa(m_proxy->paintDevice());
    pa.setRenderHints(QPainter::SmoothPixmapTransform);
    pa.setCompositionMode(QPainter::CompositionMode_Source);
    pa.drawImage(m_dirtyWindowRect, m_image, m_dirtyRect);
    pa.end();

    m_proxy->endPaint();
}

DPP_END_NAMESPACE
