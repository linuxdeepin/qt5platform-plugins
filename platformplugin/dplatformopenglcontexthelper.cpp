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

#include "dplatformopenglcontexthelper.h"
#include "vtablehook.h"
#include "dplatformwindowhelper.h"
#include "dframewindow.h"
#include "dwmsupport.h"

#include <qpa/qplatformsurface.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformbackingstore.h>

#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QOpenGLFunctions>
#include <QPainterPathStroker>
#include <QDebug>

DPP_BEGIN_NAMESPACE

DPlatformOpenGLContextHelper::DPlatformOpenGLContextHelper()
{

}

bool DPlatformOpenGLContextHelper::addOpenGLContext(QOpenGLContext *object, QPlatformOpenGLContext *context)
{
    QObject::connect(object, &QObject::destroyed, object, [context] {
        VtableHook::clearGhostVtable(context);
    });

    return VtableHook::overrideVfptrFun(context, &QPlatformOpenGLContext::swapBuffers, this, &DPlatformOpenGLContextHelper::swapBuffers);
}

void DPlatformOpenGLContextHelper::swapBuffers(QPlatformSurface *surface)
{
    if (!DWMSupport::instance()->hasComposite())
        goto end;

    if (surface->surface()->surfaceClass() == QSurface::Window) {
        QWindow *window = static_cast<QWindow*>(surface->surface());
        DPlatformWindowHelper *window_helper = DPlatformWindowHelper::mapped.value(window->handle());

        if (!window_helper)
            goto end;

        if (!window_helper->m_isUserSetClipPath && window_helper->m_windowRadius <= 0)
            goto end;

        qreal device_pixel_ratio = window_helper->m_nativeWindow->window()->devicePixelRatio();
        QPainterPath path;
        const QPainterPath &real_clip_path = window_helper->getClipPath() * device_pixel_ratio;
        const QSize &window_size = window->handle()->geometry().size();

        path.addRect(QRect(QPoint(0, 0), window_size));
        path -= real_clip_path;

        if (path.isEmpty())
            goto end;

        QOpenGLPaintDevice device(window_size);
        QPainter pa_device(&device);
        const QRect &rect = QRect(window_helper->m_frameWindow->contentOffsetHint() * device_pixel_ratio, window_size);

        QBrush border_brush(window_helper->m_frameWindow->platformBackingStore->toImage());

        border_brush.setMatrix(QMatrix(1, 0, 0, 1, -rect.x(), -rect.y()));

        pa_device.setCompositionMode(QPainter::CompositionMode_Source);
        pa_device.fillPath(path, border_brush);
        pa_device.end();
    }

end:
    VtableHook::callOriginalFun(this->context(), &QPlatformOpenGLContext::swapBuffers, surface);
}

DPP_END_NAMESPACE
