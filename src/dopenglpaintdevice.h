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
#ifndef DOPENGLPAINTDEVICE_H
#define DOPENGLPAINTDEVICE_H

#include "global.h"

#include <QOffscreenSurface>
#include <QOpenGLPaintDevice>
#include <QImage>

DPP_BEGIN_NAMESPACE

class DOpenGLPaintDevicePrivate;
class DOpenGLPaintDevice : public QOpenGLPaintDevice
{
    Q_DECLARE_PRIVATE(DOpenGLPaintDevice)

public:
    enum UpdateBehavior {
        NoPartialUpdate,
        PartialUpdateBlit,
        PartialUpdateBlend
    };

    explicit DOpenGLPaintDevice(QSurface *surface, UpdateBehavior updateBehavior = PartialUpdateBlit);
    explicit DOpenGLPaintDevice(const QSize &size, UpdateBehavior updateBehavior = PartialUpdateBlit);
    explicit DOpenGLPaintDevice(QOpenGLContext *shareContext, QSurface *surface = nullptr, UpdateBehavior updateBehavior = PartialUpdateBlit);
    explicit DOpenGLPaintDevice(QOpenGLContext *shareContext, const QSize &size, UpdateBehavior updateBehavior = PartialUpdateBlit);
    ~DOpenGLPaintDevice();

    void resize(const QSize &size);

    UpdateBehavior updateBehavior() const;
    bool isValid() const;

    void makeCurrent();
    void doneCurrent();
    void flush();

    QOpenGLContext *context() const;
    QOpenGLContext *shareContext() const;

    GLuint defaultFramebufferObject() const;

    QImage grabFramebuffer();

private:
    using QOpenGLPaintDevice::setSize;
    void ensureActiveTarget() override;

    Q_DISABLE_COPY(DOpenGLPaintDevice)
};

DPP_END_NAMESPACE

#endif // DOPENGLPAINTDEVICE_H
