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
#ifndef DBACKINGSTOREPROXY_H
#define DBACKINGSTOREPROXY_H

#include "global.h"

#include <qpa/qplatformbackingstore.h>

DPP_BEGIN_NAMESPACE

class DOpenGLPaintDevice;
class DBackingStoreProxy : public QPlatformBackingStore
{
public:
    static bool useGLPaint(const QObject *obj);

    DBackingStoreProxy(QPlatformBackingStore *proxy, bool useGLPaint = false);
    ~DBackingStoreProxy() override;

    QPaintDevice *paintDevice() override;

    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
#ifndef QT_NO_OPENGL
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                         QPlatformTextureList *textures, QOpenGLContext *context) override;
#elif QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                         QPlatformTextureList *textures, QOpenGLContext *context,
                         bool translucentBackground) override;
#else
    void composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                         QPlatformTextureList *textures,
                         bool translucentBackground) override;
#endif
    GLuint toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const override;
#endif
    QImage toImage() const override;
    QPlatformGraphicsBuffer *graphicsBuffer() const override;

    void resize(const QSize &size, const QRegion &staticContents) override;

    bool scroll(const QRegion &area, int dx, int dy) override;

    void beginPaint(const QRegion &region) override;
    void endPaint() override;

private:
    QPlatformBackingStore *m_proxy = nullptr;
    QImage m_image;
    QRectF m_dirtyWindowRect;
    QRect m_dirtyRect;

    QScopedPointer<DOpenGLPaintDevice> glDevice;
    bool enableGL = false;
};

DPP_END_NAMESPACE

#endif // DBACKINGSTOREPROXY_H
