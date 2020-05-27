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
#include <qpa/qplatformbackingstore.h>

QT_BEGIN_NAMESPACE
class QWindow;
class QXcbScreen;
class QXcbVirtualDesktop;
class QPlatformWindow;
class QXcbBackingStore;
QT_END_NAMESPACE

typedef struct xcb_connection_t xcb_connection_t;

DPP_BEGIN_NAMESPACE

class HighDpiImage;
class DHighDpi
{
public:
    static QPointF fromNativePixels(const QPointF &pixelPoint, const QWindow *window);

    static void init();
    static bool isActive();
    static bool overrideBackingStore();
    static QDpi logicalDpi(QXcbScreen *s);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    static qreal pixelDensity(QXcbScreen *s);
#endif
    static qreal devicePixelRatio(QPlatformWindow *w);
    // for backingstore
    class BackingStore : public QPlatformBackingStore
    {
    public:
        BackingStore(QPlatformBackingStore *proxy);
        ~BackingStore() override;

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
    };

    static void onDPIChanged(xcb_connection_t *screen, const QByteArray &name, const QVariant &property, void *handle);

private:

    static bool active;
    static QHash<QPlatformScreen*, qreal> screenFactorMap;
};

DPP_END_NAMESPACE

#endif // DHIGHDPI_H
